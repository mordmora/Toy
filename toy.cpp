
#include <string>
#include <memory>
#include <vector>

enum Token{
    tok_eof = -1,
    
    //comandos
    tok_def = -2,
    tok_extern = -3,

    //identificadores
    tok_identifier = -4,
    tok_number = -5
};

static std::string identifierStr;
static int NumVal;

static int gettok(){
    static int LastChar = ' ';

    while(isspace(LastChar))
        LastChar = getchar();


    if(isalpha(LastChar)){  //identificador [a-zA-Z][a-zA-Z0-9]*
        identifierStr = LastChar;
        while(isalnum(LastChar = getchar()))
            identifierStr += LastChar;
        
        if(identifierStr == "define")
            return tok_def;
        if(identifierStr == "extern")
            return tok_extern;

        return tok_identifier;
    }

    if(isdigit(LastChar) || LastChar == '.'){
        std::string NumStr;
        do{
            NumStr += LastChar;
            LastChar = getchar();
            
        } while(isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if(LastChar == '#'){
        do{
            LastChar = getchar();

        }while(LastChar != EOF && LastChar != '\n' && LastChar != '\r');
        if(LastChar != EOF)
            return gettok();
    }

    if(LastChar == 'EOF')
        return tok_eof;
    
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

class ExprAST { //Clase base para representar los nodos 
    public:
        virtual ~ExprAST() = default;
};

class NumberExprAST : public ExprAST { //Subclase para representar la expresión de un numero, por ejemplo "12.4"
    double Val;
    public:
        NumberExprAST(double val) : Val(Val){}
};

class VariableExprAST : public ExprAST { //Subclase para representar una variable, por ejemplo "foo"
    std::string Name;
    public:
        VariableExprAST(const std::string &Name) : Name(Name){}
};

class BinaryExprAST : public ExprAST { //Subclase para representar una expresión binaria
    char Op;
    std::unique_ptr<ExprAST> LHS, RHS; //LHS = left hand side, RHS = right hand side
    public:
        BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)){}  
};

class CallExprAST : public ExprAST { //Subclase para representar una llamada a una funcion
    std::string Callee;
    std::vector<std::unique_ptr<ExprAST>> Args;
    public:
        CallExprAST(const std::string &Callee, std::vector<std::unique_ptr<ExprAST>> Args) :
            Callee(Callee), Args(std::move(Args)){}
};

class PrototypeAST : public ExprAST { 
   /*
    Subclase para representar los prototipos de una funcion, toma el nombre y los argumentos (implicitamente el numero de argumentos que la funcion toma)
   */
    std::string Name;
    std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)){}
};

class FunctionAST : public ExprAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto, std::unique_ptr<ExprAST> Body) 
            : Proto(std::move(Proto)), Body(std::move(Body)){}
};

auto LHS = std::make_unique<VariableExprAST>("x");
auto RHS = std::make_unique<VariableExprAST>("y");
auto Result = std::make_unique<BinaryExprAST>('+', std::move(LHS), std::move(RHS));

static int CurrTok;
int getNextToken(){ //buffer de tokens
    return CurrTok = gettok();
}

//Manejo de errores
std::unique_ptr<ExprAST> LogError(const char *Str){
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* Str){
    LogError(Str);
    return nullptr;
}

//Parsing

// numExpr => num
static std::unique_ptr<ExprAST> ParseNumberExpr(){
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}


// parenExpr => ( expr )
static std::unique_ptr<ExprAST> ParseParenExpr(){
    getNextToken(); // come (
    auto V = ParseExpression();
    if(!V)
        return nullptr;
    
    if(CurrTok != ')')
        return LogError("Se esperaba ')' ");
    getNextToken(); //come )
    return V;
}

//identifierExpr => identifier | identifier '(' expression ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr(){
    std::string IdName = identifierStr;

    getNextToken();
    if(CurrTok != '(') // referencia a variable normal
        return std::make_unique<VariableExprAST>(IdName);
    
    getNextToken();
    std::vector<std::unique_ptr<ExprAST>> Args;
    if(CurrTok != ')' ){
        while(true){
            if(auto Arg = ParseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if(CurrTok == ')')
                break;
            
            if(CurrTok != ',')
                return LogError("Se esperaba ',' o ')' en la lista de argumentos");
            getNextToken();
        }
    }
    getNextToken(); //Come )
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}