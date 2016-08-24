#include "Context.h"

#include "../containers/Ref.h"
#include "../math/Rand.h"
#include "../streams/FileStream.h"
#include "../math/Vector.h"
#include "../system/Window.h"
#include "../time/Time.h"

using namespace L;
using namespace Script;

Table<Symbol,Var> Context::_globals;
Table<const TypeDescription*,Var> Context::_typeTables;

void Context::read(Stream& stream) {
  Script::Lexer lexer(stream);
  Var v;
  lexer.nextToken();
  while(!stream.end()) {
    read(v,lexer);
    execute(v);
  }
}
void Context::read(Var& v,Lexer& lexer) {
  if(lexer.acceptToken("(")) { // It's a list of expressions
    v.make<Array<Var> >();
    int i(0);
    while(!lexer.acceptToken(")"))
      if(lexer.acceptToken("|"))
        v = Array<Var>{v}, i = 1;
      else read(v[i++],lexer);
  } else if(lexer.acceptToken("'")) {
    read(v.make<Quote>().var,lexer);
  } else if(lexer.acceptToken("!")) {
    read(v,lexer);
    v = execute(v);
  } else {
    const char* token(lexer.token());
    if(lexer.literal()) v = token; // Character string
    else if(strpbrk(token,"0123456789")){ // Has digits
      if(token[strspn(token,"-0123456789")]=='\0') v = atoi(token); // Integer
      else if(token[strspn(token,"-0123456789.")]=='\0') v = (float)atof(token); // Float
      else v = fnv1a(token);
    } else if(lexer.isToken("true")) v = true;
    else if(lexer.isToken("false")) v = false;
    else v = fnv1a(token);
    lexer.nextToken();
  }
}

Var& Context::variable(Symbol sym) {
  // Search symbol in locals and sub-locals
  for(uintptr_t i(0); i<_stack.size(); i++)
    if(_stack[i].key()==sym)
      return _stack[i].value();
  // Search symbol in globals
  return _globals[sym];
}
Var& Context::local(Symbol sym){
  _stack.push(SymbolVar(sym));
  return _stack.top().value();
}
Var Context::execute(const Var& code,Var* src) {
  if(code.is<Array<Var> >()) {
    const Array<Var>& array(code.as<Array<Var> >()); // Get reference of array value
    Var source(false);
    const Var& handle(execute(array[0],&source)); // Execute first child of array to get function handle
    if(handle.is<Native>())
      return handle.as<Native>()(*this,array);
    else if(handle.is<Function>() || handle.is<Ref<CodeFunction> >()) {
      Var wtr;
      size_t frame(_stack.size()); // Save local frame
      if(!source.is<bool>() && handle.is<Ref<CodeFunction> >())
        _stack.push(FNV1A("self"),source);
      SymbolVar* stack(&_stack.top()+1);
      for(uint32_t i(1); i<array.size(); i++) { // For all parameters
        Symbol sym;
        if(handle.is<Ref<CodeFunction> >() && handle.as<Ref<CodeFunction> >()->parameters.size()>=i)
          sym = handle.as<Ref<CodeFunction> >()->parameters[i-1];
        else sym = FNV1A("");
        _stack.push(sym,execute(array[i])); // Compute parameter values
      }
      if(handle.is<Ref<CodeFunction> >())
        wtr = execute(handle.as<Ref<CodeFunction> >()->code);
      else if(handle.is<Function>()) // It's a function pointer
        wtr = handle.as<Function>()(source,stack,array.size()-1); // Call function
      _stack.size(frame); // Resize to the previous frame
      return wtr;
    } else if(array.size()>1){
      Ref<Table<Var,Var>> table;
      if(src) *src = handle;
      if(handle.is<Ref<Table<Var,Var>>>())
        table = handle.as<Ref<Table<Var,Var>>>();
      else
        table = typeTable(handle.type());
      return (*table)[execute(array[1])];
    }
  } else if(code.is<Symbol>()) return variable(code.as<Symbol>()); // It's a simple variable
  else if(code.is<Quote>()) return code.as<Quote>().var; // Return raw data
  return code;
}
Var* Context::reference(const Var& code,Var* src) {
  if(code.is<Symbol>()) // It's a symbol so it's a simple reference to a variable
    return &variable(code.as<Symbol>());
  else if(code.is<Array<Var> >()){ // It's an array so it may be a reference to an object field or a
    const Array<Var>& array(code.as<Array<Var> >());
    if(array.size()==2){ // It's a pair
      Var first(execute(array[0]));
      Ref<Table<Var,Var>> table;
      if(src) *src = first;
      if(first.is<Ref<Table<Var,Var>>>()){ // First is a regular table
        table = first.as<Ref<Table<Var,Var>>>();
      } else if(!first.is<Ref<CodeFunction>>() // First is not any kind of function
                && !first.is<Native>()
                && !first.is<Function>()){
        table = typeTable(first.type());
      } else return nullptr;
      return &(*table)[execute(array[1])]; // Compute index and return pointer to field
    }
  }
  return nullptr;
}

Ref<Table<Var,Var>> Context::typeTable(const TypeDescription* td){
  Var& tt(_typeTables[td]);
  if(!tt.is<Ref<Table<Var,Var>>>())
    tt = ref<Table<Var,Var>>();
  return tt.as<Ref<Table<Var,Var>>>();
}

Context::Context(){
  L_ONCE;
  // Local allows to define local variables without overriding more global variables
  _globals[FNV1A("local")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()>1 && a[1].is<Symbol>()){
      if(a.size()>2)
        c.local(a[1].as<Symbol>()) = c.execute(a[2]);
      else c.local(a[1].as<Symbol>());
    }
    return 0;
  });
  _globals[FNV1A("do")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    for(uintptr_t i(1); i<a.size()-1; i++)
      c.execute(a[i]);
    return c.execute(a.back());
  });
  _globals[FNV1A("while")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    Var wtr;
    while(c.execute(a[1]).get<bool>())
      wtr = c.execute(a[2]);
    return wtr;
  });
  _globals[FNV1A("if")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()>2) {
      if(c.execute(a[1]).get<bool>())
        return c.execute(a[2]);
      else if(a.size()>3)
        return c.execute(a[3]);
    }
    return 0;
  });
  _globals[FNV1A("set")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    Var* target;
    if(a.size()==3 && (target = c.reference(a[1])))
      *target = c.execute(a[2]);
    return 0;
  });
  _globals[FNV1A("fun")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()>1){
      Ref<CodeFunction> wtr;
      wtr.make();
      if(a.size()>2 && a[1].is<Array<Var> >()) // There's a list of parameter symbols
        for(auto&& sym : a[1].as<Array<Var> >())
          if(sym.is<Symbol>())
            wtr->parameters.push(sym.as<Symbol>());
      wtr->code = a.back(); // The last part is always the code
      return wtr;
    }
    return 0;
  });
#define CMP(name,cop)\
  _globals[FNV1A(name)] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {\
    L_ASSERT(params>=2);\
    for(uintptr_t i(0); i<params-1; i++)\
      if(stack[i].value() cop stack[i+1].value())\
        return false;\
    return true;\
  })
  CMP("=",!=);
  CMP("<>",==);
  CMP(">",<=);
  CMP("<",>=);
  CMP(">=",<);
  CMP("<=",>);
#undef CMP
  _globals[FNV1A("+")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      wtr += stack[i].value();
    return wtr;
  });
  _globals[FNV1A("*")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      wtr *= stack[i].value();
    return wtr;
  });
  _globals[FNV1A("-")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    if(params==1)
      return Var(0) - stack[0].value(); // TODO: replace with actual neg dynamic operator
    else{
      Var wtr(stack[0].value());
      for(uintptr_t i(1); i<params; i++)
        wtr -= stack[i].value();
      return wtr;
    }
  });
  _globals[FNV1A("/")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==2);
    return stack[0].value()/stack[1].value();
  });
  _globals[FNV1A("%")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==2);
    return stack[0].value()%stack[1].value();
  });
  _globals[FNV1A("print")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    for(uintptr_t i(0); i<params; i++)
      out << stack[i].value();
    return 0;
  });
  _globals[FNV1A("typename")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    return stack[0]->type()->name;
  });
  _globals[FNV1A("object")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    Ref<Table<Var,Var> > wtr(ref<Table<Var,Var>>());
    Table<Var,Var>& table(*wtr);
    for(uintptr_t i(1); i<params; i += 2)
      table[stack[i-1].value()] = stack[i].value();
    return wtr;
  });
  _globals[FNV1A("now")] = (Native)([](Context& c,const Array<Var>&)->Var {
    return Time::now();
  });
  _globals[FNV1A("rand")] = (Native)([](Context& c,const Array<Var>&)->Var {
    return Rand::nextFloat();
  });
  _globals[FNV1A("key-pressed")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()==2){
      if(a[1].is<Symbol>())
        switch(a[1].as<Symbol>()) {
#define KEY_BIND(k) case FNV1A(#k): return Window::isPressed(Window::Event::k)
          KEY_BIND(A); KEY_BIND(B); KEY_BIND(C); KEY_BIND(D); KEY_BIND(E); KEY_BIND(F);
          KEY_BIND(G); KEY_BIND(H); KEY_BIND(I); KEY_BIND(J); KEY_BIND(K); KEY_BIND(L);
          KEY_BIND(M); KEY_BIND(N); KEY_BIND(O); KEY_BIND(P); KEY_BIND(Q); KEY_BIND(R);
          KEY_BIND(S); KEY_BIND(T); KEY_BIND(U); KEY_BIND(V); KEY_BIND(W); KEY_BIND(X);
          KEY_BIND(Y); KEY_BIND(Z); KEY_BIND(BACKSPACE); KEY_BIND(TAB); KEY_BIND(ENTER);
          KEY_BIND(PAUSE); KEY_BIND(CAPS); KEY_BIND(ESCAPE); KEY_BIND(SPACE); KEY_BIND(SHIFT);
          KEY_BIND(LSHIFT); KEY_BIND(RSHIFT); KEY_BIND(CTRL); KEY_BIND(LCTRL); KEY_BIND(RCTRL);
          KEY_BIND(ALT); KEY_BIND(LALT); KEY_BIND(RALT); KEY_BIND(LEFT); KEY_BIND(RIGHT);
          KEY_BIND(UP); KEY_BIND(DOWN); KEY_BIND(PAGEUP); KEY_BIND(PAGEDOWN); KEY_BIND(END);
          KEY_BIND(HOME); KEY_BIND(NUMLOCK); KEY_BIND(NUM0); KEY_BIND(NUM1); KEY_BIND(NUM2);
          KEY_BIND(NUM3); KEY_BIND(NUM4); KEY_BIND(NUM5); KEY_BIND(NUM6); KEY_BIND(NUM7);
          KEY_BIND(NUM8); KEY_BIND(NUM9); KEY_BIND(F1); KEY_BIND(F2); KEY_BIND(F3); KEY_BIND(F4);
          KEY_BIND(F5); KEY_BIND(F6); KEY_BIND(F7); KEY_BIND(F8); KEY_BIND(F9); KEY_BIND(F10);
          KEY_BIND(F11); KEY_BIND(F12); KEY_BIND(LBUTTON); KEY_BIND(RBUTTON); KEY_BIND(MBUTTON);
        } else if(a[1].is<int>())
          return Window::isPressed((Window::Event::Button)(a[1].as<int>()+'0'));
    }
    return false;
  });
  _globals[FNV1A("vec")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params==3)
      return Vector3f(stack[0]->get<float>(),stack[1]->get<float>(),stack[2]->get<float>());
    return 0;
  });
}
