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

static Var object(const Var&,SymbolVar* stack,size_t params) {
  Ref<Table<Var,Var> > wtr(ref<Table<Var,Var>>());
  Table<Var,Var>& table(*wtr);
  for(uintptr_t i(1); i<params; i += 2)
    table[stack[i-1].value()] = stack[i].value();
  return wtr;
}

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
  if(lexer.eos())
    L_ERROR("Unexpected end of stream while parsing script: there are uneven () or {}.");
  if(lexer.acceptToken("(")) { // It's a list of expressions
    v.make<Array<Var> >();
    int i(0);
    while(!lexer.acceptToken(")"))
      if(lexer.acceptToken("|"))
        v = Array<Var>{v},i = 1;
      else read(v[i++],lexer);
  } else if(lexer.acceptToken("{")) { // It's an object
    v.make<Array<Var> >();
    v[0] = (Function)object;
    int i(1);
    while(!lexer.acceptToken("}"))
      read(v[i++],lexer);
  } else if(lexer.acceptToken("'")) {
    read(v.make<Quote>().var,lexer);
  } else if(lexer.acceptToken("!")) {
    read(v,lexer);
    v = execute(v);
  } else if(lexer.isToken(")") || lexer.isToken("}")){
    L_ERRORF("Unexpected token %s at line %d",lexer.token(),lexer.line());
  } else {
    const char* token(lexer.token());
    if(lexer.literal()) v = token; // Character string
    else if(strpbrk(token,"0123456789")){ // Has digits
      if(token[strspn(token,"-0123456789")]=='\0') v = atoi(token); // Integer
      else if(token[strspn(token,"-0123456789.")]=='\0') v = (float)atof(token); // Float
      else v = Symbol(token);
    } else if(lexer.isToken("true")) v = true;
    else if(lexer.isToken("false")) v = false;
    else v = Symbol(token);
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
        _stack.push(Symbol("self"),source);
      SymbolVar* stack(&_stack.top()+1);
      for(uint32_t i(1); i<array.size(); i++) { // For all parameters
        Symbol sym;
        if(handle.is<Ref<CodeFunction> >() && handle.as<Ref<CodeFunction> >()->parameters.size()>=i)
          sym = handle.as<Ref<CodeFunction> >()->parameters[i-1];
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
  _globals[Symbol("local")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()>1 && a[1].is<Symbol>()){
      if(a.size()>2)
        c.local(a[1].as<Symbol>()) = c.execute(a[2]);
      else c.local(a[1].as<Symbol>());
    }
    return 0;
  });
  _globals[Symbol("do")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    for(uintptr_t i(1); i<a.size()-1; i++)
      c.execute(a[i]);
    return c.execute(a.back());
  });
  _globals[Symbol("and")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    for(uintptr_t i(1); i<a.size(); i++)
      if(!c.execute(a[i]).get<bool>())
        return false;
    return true;
  });
  _globals[Symbol("not")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==1);
    return !stack[0].value().get<bool>();
  });
  _globals[Symbol("non-null")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==1);
    return stack[0].value().as<void*>()!=nullptr;
  });
  _globals[Symbol("count")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==1);
    if(stack[0]->is<Ref<Table<Var,Var>>>())
      return (int)stack[0]->as<Ref<Table<Var,Var>>>()->count();
    return 0;
  });
  _globals[Symbol("or")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    for(uintptr_t i(1); i<a.size(); i++)
      if(c.execute(a[i]).get<bool>())
        return true;
    return false;
  });
  _globals[Symbol("while")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    Var wtr;
    while(c.execute(a[1]).get<bool>())
      wtr = c.execute(a[2]);
    return wtr;
  });
  _globals[Symbol("foreach")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    L_ASSERT(a.size()>=4);
    Var *key(nullptr),*value;
    const Var* exec;
    Var tableVar;
    if(a.size()==4){ // Value only
      value = &c.local(a[1].as<Symbol>());
      tableVar = c.execute(a[2]);
      exec = &a[3];
    } else { // Key value
      key = &c.local(a[1].as<Symbol>());
      value = &c.local(a[2].as<Symbol>());
      tableVar = c.execute(a[3]);
      exec = &a[4];
    }
    for(auto&& slot : *tableVar.as<Ref<Table<Var,Var>>>()){
      if(key) *key = slot.key();
      *value = slot.value();
      c.execute(*exec);
    }
    return Var();
  });
  _globals[Symbol("if")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    for(uintptr_t i(1); i<a.size()-1; i += 2)
      if(c.execute(a[i]).get<bool>())
        return c.execute(a[i+1]);
    if(!(a.size()&1))
      return c.execute(a.back());
    else return 0;
  });
  _globals[Symbol("switch")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    if(a.size()>1){
      const Var v(c.execute(a[1]));
      for(uintptr_t i(2); i<a.size()-1; i += 2)
        if(c.execute(a[i])==v)
          return c.execute(a[i+1]);
      if(a.size()&1)
        return c.execute(a.back());
    }
    return 0;
  });
  _globals[Symbol("set")] = (Native)([](Context& c,const Array<Var>& a)->Var {
    L_ASSERT(a.size()==3);
    return *c.reference(a[1]) = c.execute(a[2]);
  });
  _globals[Symbol("fun")] = (Native)([](Context& c,const Array<Var>& a)->Var {
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
  _globals[Symbol(name)] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {\
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
  _globals[Symbol("max")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      if(stack[i].value()>wtr)
        wtr = stack[i].value();
    return wtr;
  });
  _globals[Symbol("min")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      if(stack[i].value()<wtr)
        wtr = stack[i].value();
    return wtr;
  });
#define SETOP(op)\
  _globals[Symbol(#op)] = (Native)([](Context& c,const Array<Var>& a)->Var {\
    L_ASSERT(a.size()>1);\
    Var* target(c.reference(a[1]));\
    for(uintptr_t i(2);i<a.size();i++)\
      *target op c.execute(a[i]);\
    return 0;\
  })
  SETOP(+= );
  SETOP(-= );
  SETOP(*= );
  SETOP(/= );
  SETOP(%= );
#undef SETOP
  _globals[Symbol("+")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      wtr += stack[i].value();
    return wtr;
  });
  _globals[Symbol("*")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params>=1);
    Var wtr(stack[0].value());
    for(uintptr_t i(1); i<params; i++)
      wtr *= stack[i].value();
    return wtr;
  });
  _globals[Symbol("-")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
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
  _globals[Symbol("/")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==2);
    return stack[0].value()/stack[1].value();
  });
  _globals[Symbol("%")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    L_ASSERT(params==2);
    return stack[0].value()%stack[1].value();
  });
  _globals[Symbol("print")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    for(uintptr_t i(0); i<params; i++)
      out << stack[i].value();
    return 0;
  });
  _globals[Symbol("typename")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    return stack[0]->type()->name;
  });
  _globals[Symbol("object")] = (Function)object;
  _globals[Symbol("now")] = (Native)([](Context& c,const Array<Var>&)->Var {
    return Time::now();
  });
  _globals[Symbol("rand")] = (Native)([](Context& c,const Array<Var>&)->Var {
    return Rand::nextFloat();
  });
  _globals[Symbol("button-pressed")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params){
      if(stack[0]->is<Symbol>())
        return Window::isPressed(Window::symbolToButton(stack[0]->as<Symbol>()));
      else if(stack[0]->is<int>())
        return Window::isPressed((Window::Event::Button)(stack[0]->as<int>()+'0'));
    }
    return false;
  });
  _globals[Symbol("vec")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    if(params==3)
      return Vector3f(stack[0]->get<float>(),stack[1]->get<float>(),stack[2]->get<float>());
    return Vector3f();
  });
  _globals[Symbol("color")] = (Function)([](const Var&,SymbolVar* stack,size_t params)->Var {
    Color wtr(Color::white);
    params = min(params,4u);
    for(size_t i(0); i<params; i++){
      byte b((stack[i]->is<float>()) ? (stack[i]->as<float>()*255) : stack[i]->get<int>());
      switch(i){
        case 0: wtr.r() = b; break;
        case 1: wtr.g() = b; break;
        case 2: wtr.b() = b; break;
        case 3: wtr.a() = b; break;
      }
    }
    return wtr;
  });
}
