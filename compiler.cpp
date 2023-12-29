#include "compiler.h"

#include "common.h"
#include "memory.h"
#include "object.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>

struct Parser {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
};

// clang-format off
enum Precedence  {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY,
};
// clang-format on

typedef void (*ParseFn)(bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int depth;
    bool isCaptured;
};

struct Upvalue {
    uint8_t index;
    bool isLocal;
};

enum FunctionType {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT,
};

struct Compiler {
    Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;
    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
};

struct ClassCompiler {
    ClassCompiler* enclosing;
};

Parser parser;
Compiler* current = nullptr;
ClassCompiler* currentClass = nullptr;

static Chunk*
currentChunk() {
    return &(current->function->chunk);
}

static void
errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}

static void
error(const char* message) {
    errorAt(&parser.previous, message);
}

static void
errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void
advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
            break;

        errorAtCurrent(parser.current.start);
    }
}

static void
consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool
check(TokenType type) {
    return parser.current.type == type;
}

static bool
match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static void
emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void
emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void
emitLoop(int loopStart) {
    emitByte(OpCode::OP_POP);

    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int
emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}

static void
emitReturn() {
    if (current->type == FunctionType::TYPE_INITIALIZER) {
        emitBytes(OpCode::OP_GET_LOCAL, 0);
    } else {
        emitByte(OpCode::OP_NIL);
    }

    emitByte(OpCode::OP_RETURN);
}

static uint8_t
makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static ObjFunction*
endCompiler() {
    emitReturn();
    ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), function->name != nullptr ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void
beginScope() {
    current->scopeDepth += 1;
}

static void
endScope() {
    current->scopeDepth -= 1;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OpCode::OP_CLOSE_UPVALUE);
        } else {
            emitByte(OpCode::OP_POP);
        }
        current->localCount -= 1;
    }
}

static void
expression();

static void
statement();

static void
declaration();

static uint8_t
argumentList();

static ParseRule*
getRule(TokenType type);

static void
parsePrecedence(Precedence precedence);

// clang-format off
static void
binary(bool canAssign) {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
    }
}

static void
call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OpCode::OP_CALL, argCount);
}

static uint8_t
identifierConstant(Token* name);

static void
dot(bool canAssign) {
    consume(TokenType::TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&(parser.previous));

    if (canAssign && match(TokenType::TOKEN_EQUAL)) {
        expression();
        emitBytes(OpCode::OP_SET_PROPERTY, name);
    } else if (match(TokenType::TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OpCode::OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OpCode::OP_GET_PROPERTY, name);
    }
}

static void
literal(bool canAssign) {
    switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
    }
}
// clang-format on

static void
grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void
emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void
patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}

static void
initCompiler(Compiler* compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = nullptr;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != FunctionType::TYPE_SCRIPT) {
        current->function->name = copyString(parser.previous.start, parser.previous.length);
    }

    Local* local = &(current->locals[current->localCount++]);
    local->depth = 0;
    local->isCaptured = false;
    if (type != FunctionType::TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static void
number(bool canAssign) {
    double value = strtod(parser.previous.start, nullptr);
    emitConstant(NUMBER_VAL(value));
}

static void
and_(bool canAssign) {
    int endJump = emitJump(OpCode::OP_JUMP_IF_FALSE);

    emitByte(OpCode::OP_POP);
    parsePrecedence(Precedence::PREC_AND);

    patchJump(endJump);
}

static void
or_(bool canAssign) {
    int elseJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    int endJump = emitJump(OpCode::OP_JUMP);

    patchJump(elseJump);
    emitByte(OpCode::OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}

static void
string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static uint8_t
identifierConstant(Token* name);
static int
resolveLocal(Compiler* compiler, Token* name);
static int
resolveUpvalue(Compiler* compiler, Token* name);

static void
namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OpCode::OP_GET_LOCAL;
        setOp = OpCode::OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OpCode::OP_GET_UPVALUE;
        setOp = OpCode::OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OpCode::OP_GET_GLOBAL;
        setOp = OpCode::OP_SET_GLOBAL;
    }

    if (canAssign && match(TokenType::TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void
variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void
this_(bool canAssign) {
    if (currentClass == nullptr) {
        error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
}

// clang-format off
static void
unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
    }
}
// clang-format on

// clang-format off
ParseRule rules[] = {
  [TOKEN_LEFT_PAREN]    = {grouping, call,   PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
  [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_DOT]           = {NULL,     dot,    PREC_CALL},
  [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
  [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
  [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,     binary, PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable, NULL,   PREC_NONE},
  [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
  [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
  [TOKEN_AND]           = {NULL,     and_,   PREC_AND},
  [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
  [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
  [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
  [TOKEN_OR]            = {NULL,     or_,    PREC_OR},
  [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
  [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_THIS]          = {this_,    NULL,   PREC_NONE},
  [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
  [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
  [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
  [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};
// clang-format on

static void
parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TokenType::TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t
identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool
identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static int
resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &(compiler->locals[i]);
        if (identifiersEqual(name, &(local->name))) {
            if (local->depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int
addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &(compiler->upvalues[i]);
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int
resolveUpvalue(Compiler* compiler, Token* name) {
    if (compiler->enclosing == nullptr) {
        return -1;
    }

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (int8_t)upvalue, false);
    }

    return -1;
}

static void
addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &(current->locals[current->localCount++]);
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void
declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token* name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &(current->locals[i]);
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &(local->name))) {
            error("Already a variable with this name in this scope.");
        }
    }
    addLocal(*name);
}

static uint8_t
parseVariable(const char* errorMessage) {
    consume(TokenType::TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&(parser.previous));
}

static void
markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void
defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OpCode::OP_DEFINE_GLOBAL, global);
}

static uint8_t
argumentList() {
    uint8_t argCount = 0;
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount += 1;
        } while (match(TokenType::TOKEN_COMMA));
    }
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static ParseRule*
getRule(TokenType type) {
    return &rules[type];
}

static void
expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void
block() {
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !check(TokenType::TOKEN_EOF)) {
        declaration();
    }

    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '}' after block");
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope();

    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TokenType::TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity += 1;
            if (current->function->arity > 255) {
                errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (match(TokenType::TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction* function = endCompiler();
    emitBytes(OpCode::OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void
method() {
    consume(TokenType::TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&(parser.previous));

    FunctionType type = FunctionType::TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = FunctionType::TYPE_INITIALIZER;
    }
    function(type);
    emitBytes(OpCode::OP_METHOD, constant);
}

static void
classDeclaration() {
    consume(TokenType::TOKEN_IDENTIFIER, "Expect class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifierConstant(&(parser.previous));
    declareVariable();

    emitBytes(OpCode::OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    namedVariable(className, false);
    consume(TokenType::TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TokenType::TOKEN_RIGHT_BRACE) && !check(TokenType::TOKEN_EOF)) {
        method();
    }
    consume(TokenType::TOKEN_RIGHT_BRACE, "Expect '{' after class body.");
    emitByte(OpCode::OP_POP);

    currentClass = currentClass->enclosing;
}

static void
funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(FunctionType::TYPE_FUNCTION);
    defineVariable(global);
}

static void
varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TokenType::TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OpCode::OP_NIL);
    }
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
    defineVariable(global);
}

static void
expressionStatement() {
    expression();
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OpCode::OP_POP);
}

static void
forStatement() {
    beginScope();
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TokenType::TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (match(TokenType::TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TokenType::TOKEN_SEMICOLON)) {
        expression();
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
        emitByte(OpCode::OP_POP);
    }

    if (!match(TokenType::TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OpCode::OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OpCode::OP_POP);
        consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OpCode::OP_POP);
    }

    endScope();
}

static void
ifStatement() {
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);
    statement();

    int elseJump = emitJump(OpCode::OP_JUMP);

    patchJump(thenJump);
    emitByte(OpCode::OP_POP);

    if (match(TokenType::TOKEN_ELSE)) {
        statement();
    }

    patchJump(elseJump);
}

static void
printStatement() {
    expression();
    consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OpCode::OP_PRINT);
}

static void
returnStatement() {
    if (current->type == FunctionType::TYPE_SCRIPT) {
        error("Can't return from top-level code.");
    }

    if (match(TokenType::TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == FunctionType::TYPE_INITIALIZER) {
            error("Can't return a value from an initializer.");
        }

        expression();
        consume(TokenType::TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OpCode::OP_RETURN);
    }
}

static void
whileStatement() {
    int loopStart = currentChunk()->count;
    consume(TokenType::TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TokenType::TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE);
    emitByte(OpCode::OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OpCode::OP_POP);
}

static void
synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TokenType::TOKEN_EOF) {
        if (parser.previous.type == TokenType::TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
        case TokenType::TOKEN_CLASS:
        case TokenType::TOKEN_FUN:
        case TokenType::TOKEN_VAR:
        case TokenType::TOKEN_FOR:
        case TokenType::TOKEN_IF:
        case TokenType::TOKEN_WHILE:
        case TokenType::TOKEN_PRINT:
        case TokenType::TOKEN_RETURN:
            return;
        default:; // Do nothing.
        }
    }
}

static void
declaration() {
    if (match(TokenType::TOKEN_CLASS)) {
        classDeclaration();
    } else if (match(TokenType::TOKEN_FUN)) {
        funDeclaration();
    } else if (match(TokenType::TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

static void
statement() {
    if (match(TokenType::TOKEN_PRINT)) {
        printStatement();
    } else if (match(TokenType::TOKEN_FOR)) {
        forStatement();
    } else if (match(TokenType::TOKEN_IF)) {
        ifStatement();
    } else if (match(TokenType::TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TokenType::TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TokenType::TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction*
compile(const char* source) {
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler, FunctionType::TYPE_SCRIPT);

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TokenType::TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = endCompiler();
    return parser.hadError ? nullptr : function;
}

void
markCompilerRoots() {
    Compiler* compiler = current;
    while (compiler != nullptr) {
        markObject((Obj*)(compiler->function));
        compiler = compiler->enclosing;
    }
}
