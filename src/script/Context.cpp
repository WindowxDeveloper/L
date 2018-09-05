#include "Context.h"

#include "../container/Ref.h"
#include "../engine/Resource.h"
#include "../engine/Resource.inl"
#include "../macros.h"
#include "../math/Rand.h"
#include "../stream/CFileStream.h"
#include "../math/Vector.h"
#include "../system/Window.h"
#include "../time/Time.h"

using namespace L;
using namespace Script;

Table<Symbol, Var> Context::_globals;
Table<const TypeDescription*, Var> Context::_typeTables;

static void object(Context& c) {
  Table<Var, Var>& table(c.returnValue().make<Ref<Table<Var, Var>>>().make());
  const uint32_t params(c.localCount());
  for(uint32_t i(1); i<params; i += 2)
    table[c.local(i-1)] = c.local(i);
}

Var Context::executeReturn(const Var& code) {
  const Var wtr(executeRef(code));
  _stack.pop();
  return wtr;
}
Var& Context::executeRef(const Var& code) {
  const size_t stack_size(_stack.size());
  execute(code);
  L_ASSERT(_stack.size()==stack_size+1);
  return _stack.back();
}
void Context::discardExecute(const Var& code) {
  _stack.pop();
  execute(code);
}
void Context::executeDiscard(const Var& code) {
  const size_t stack_size(_stack.size());
  execute(code);
  L_ASSERT(_stack.size()==stack_size+1);
  _stack.pop();
}
void Context::execute(const Var& code, Var* selfOut) {
  static const Symbol selfSymbol("self");
  if(code.is<Array<Var> >()) {
    const Array<Var>& array(code.as<Array<Var>>()); // Get reference of array value
    Var selfIn;
    execute(array[0], &selfIn); // Execute first child of array to get function handle
    const Var handle(_stack.back());
    if(handle.is<Native>())
      handle.as<Native>()(*this, array);
    else if(handle.is<Function>() || handle.is<Ref<CodeFunction>>() || handle.is<Resource<CodeFunction>>()) {
      for(uint32_t i(1); i<array.size(); i++) // For all parameters
        execute(array[i]); // Compute parameter values
      _frames.push(uint32_t(_stack.size()-array.size()+1)); // Save local frame
      if(!selfIn.is<void>()) _selves.push(selfIn);
      if(handle.is<Ref<CodeFunction>>() || handle.is<Resource<CodeFunction>>()) {
        const CodeFunction* function(handle.is<Ref<CodeFunction>>() ? handle.as<Ref<CodeFunction>>() : &*handle.as<Resource<CodeFunction>>());
        if(function) {
          _stack.size(currentFrame()+function->local_count);
          execute(function->code);
          returnValue() = _stack.back();
        } else returnValue() = Var();
      } else if(handle.is<Function>()) // It's a function pointer
        handle.as<Function>()(*this); // Call function
      _stack.size(_frames.back()); // Resize to the previous frame
      _frames.pop();
      if(!selfIn.is<void>()) _selves.pop();
    } else if(array.size()>1 && !handle.is<void>()) { // Table element access
      if(Var* element = reference(code, selfOut))
        _stack.back() = *element;
      else
        err << "Unable to execute table access " << array << "\n";
    } else
      err << "Unable to execute command " << array << "\n";
  } else if(code.is<Local>()) {
    // Do a copy to avoid potential reading of freed memory
    const Var value(local(code.as<Local>().i));
    _stack.push(value);
  } else if(code.is<Symbol>()) // It's a global variable or self
    _stack.push(code.as<Symbol>()==selfSymbol ? currentSelf() : global(code.as<Symbol>()));
  else if(code.is<RawSymbol>()) _stack.push(code.as<RawSymbol>().sym); // Return raw symbol
  else _stack.push(code); // Return raw value
}
Var* Context::reference(const Var& code, Var* src) {
  static const Symbol selfSymbol("self");
  if(code.is<Local>()) // It's a reference to a local variable
    return &local(code.as<Local>().i);
  if(code.is<Symbol>()) // It's a symbol so it's a reference to a global variable or self
    return (code.as<Symbol>()==selfSymbol) ? &currentSelf() : &global(code.as<Symbol>());
  else if(code.is<Array<Var> >()) { // It's an array so it may be a reference to an object field or a
    const Array<Var>& array(code.as<Array<Var> >());
    Ref<Table<Var, Var>> table;
    Var* wtr(&executeRef(array[0]));
    for(uintptr_t i(1); i<array.size(); i++) {
      if(src) *src = *wtr;
      if(wtr->is<Ref<Table<Var, Var>>>()) { // First is a regular table
        table = wtr->as<Ref<Table<Var, Var>>>();
      } else if(!wtr->is<Ref<CodeFunction>>() // First is not any kind of function or void
                && !wtr->is<Native>()
                && !wtr->is<Function>()
                && !wtr->is<void>()) {
        table = typeTable(wtr->type());
      } else if(wtr->is<void>())
        error("Trying to index from void");
      else return nullptr;
      execute(array[i]); // Compute index
      wtr = &(*table)[_stack.back()]; // Get pointer to field
      _stack.pop(); // Clean up index
    }
    _stack.pop(); // Clean up handle
    return wtr;
  }
  return nullptr;
}
bool Context::tryExecuteMethod(const Symbol& sym, std::initializer_list<Var> parameters) {
  static Var methodCall(Array<Var>(1, Var(Array<Var>{Symbol("self"), Script::RawSymbol()})));
  static Array<Var>& callArray(methodCall.as<Array<Var>>());
  static Symbol& callSym(callArray[0].as<Array<Var>>()[1].as<Script::RawSymbol>().sym);
  auto it(_self.as<Ref<Table<Var, Var>>>()->find(sym));
  if(it) {
    callSym = sym;
    callArray.size(1);
    for(auto&& p : parameters)
      callArray.push(p);
    _selves.push(_self);
    executeDiscard(methodCall);
    _selves.pop();
    return true;
  }
  return false;
}
Var Context::executeInside(const Var& code) {
  _selves.push(_self);
  execute(code);
  const Var wtr(_stack.back());
  _stack.pop();
  _selves.pop();
  return wtr;
}

Ref<Table<Var, Var>> Context::typeTable(const TypeDescription* td) {
  Var& tt(_typeTables[td]);
  if(!tt.is<Ref<Table<Var, Var>>>())
    tt = ref<Table<Var, Var>>();
  return tt.as<Ref<Table<Var, Var>>>();
}

Context::Context(const Var& self) : _self(self) {
  L_ONCE;
  _globals[Symbol("fun")] = (Native)([](Context& c, const Array<Var>& a) {
    error("fun should have been stripped during compilation");
  });
  _globals[Symbol("local")] = (Native)([](Context& c, const Array<Var>& a) {
    error("local should have been stripped during compilation");
  });
  _globals[Symbol("do")] = (Native)([](Context& c, const Array<Var>& a) {
    for(uintptr_t i(1); i<a.size(); i++)
      c.discardExecute(a[i]);
  });
  _globals[Symbol("and")] = (Native)([](Context& c, const Array<Var>& a) {
    for(uintptr_t i(1); i<a.size(); i++) {
      c.discardExecute(a[i]);
      if(!c._stack.back().get<bool>())
        return;
    }
  });
  _globals[Symbol("not")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = !c.local(0).get<bool>();
  });
  _globals[Symbol("non-null")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = !c.local(0).is<void>() &&c.local(0).as<void*>()!=nullptr;
  });
  _globals[Symbol("count")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    if(c.local(0).is<Ref<Table<Var, Var>>>())
      c.returnValue() = float(c.local(0).as<Ref<Table<Var, Var>>>()->count());
    else c.returnValue() = 0;
  });
  _globals[Symbol("or")] = (Native)([](Context& c, const Array<Var>& a) {
    for(uintptr_t i(1); i<a.size(); i++) {
      c.discardExecute(a[i]);
      if(c._stack.back().get<bool>())
        return;
    }
  });
  _globals[Symbol("while")] = (Native)([](Context& c, const Array<Var>& a) {
    L_ASSERT(a.size()==3);
    while(true) {
      c.discardExecute(a[1]);
      if(c._stack.back().get<bool>())
        c.discardExecute(a[2]);
      else return;
    }
  });
  _globals[Symbol("foreach")] = (Native)([](Context& c, const Array<Var>& a) {
    L_ASSERT(a.size()>=4);
    const Var* exec;
    Var table;
    if(a.size()==4) { // Value only
      table = c.executeReturn(a[2]);
      exec = &a[3];
    } else if(a.size()==5) { // Key value
      table = c.executeReturn(a[3]);
      exec = &a[4];
    } else error("Foreach with %d parameters is not handled", a.size());
    L_ASSERT(table.is<Ref<Table<Var, Var>>>());
    for(const auto& slot : *table.as<Ref<Table<Var, Var>>>()) {
      if(a.size()==4) { // Value only
        c.local(a[1].as<Local>().i) = slot.value();
      } else { // Key value
        c.local(a[1].as<Local>().i) = slot.key();
        c.local(a[2].as<Local>().i) = slot.value();
      }
      c.discardExecute(*exec);
    }
  });
  _globals[Symbol("if")] = (Native)([](Context& c, const Array<Var>& a) {
    for(uintptr_t i(1); i<a.size()-1; i += 2) {
      c.discardExecute(a[i]);
      if(c._stack.back().get<bool>()) {
        c.discardExecute(a[i+1]);
        return;
      }
    }
    if(!(a.size()&1))
      c.discardExecute(a.back());
  });
  _globals[Symbol("switch")] = (Native)([](Context& c, const Array<Var>& a) {
    if(a.size()>1) {
      const Var& testValue(c.executeRef(a[1])); // Compute test value
      for(uintptr_t i(2); i<a.size()-1; i += 2) {
        const Var& caseValue(c.executeRef(a[i]));
        if(caseValue==testValue) {
          c._stack.pop(); // Pop case value
          c._stack.pop(); // Pop test value
          c.discardExecute(a[i+1]);
          return;
        }
      }
      c._stack.pop(); // Pop test value
      if(a.size()&1)
        c.discardExecute(a.back());
    }
  });
  _globals[Symbol("set")] = (Native)([](Context& c, const Array<Var>& a) {
    if(a.size()==3) {
      c.discardExecute(a[2]);
      if(Var* lvalue = c.reference(a[1]))
        *lvalue = c._stack.back();
      else
        err << "Unable to assign to " << a[1] << "\n";
    }
  });
#define CMP(name,cop)\
  _globals[Symbol(name)] = (Function)([](Context& c) {\
    L_ASSERT(c.localCount()>=2);\
    for(uintptr_t i(1); i<c.localCount(); i++)\
      if(c.local(i-1) cop c.local(i)){\
        c.returnValue() = false;\
        return;\
      }\
    c.returnValue() = true;\
  })
  CMP("=", !=);
  CMP("<>", ==);
  CMP(">", <=);
  CMP("<", >=);
  CMP(">=", <);
  CMP("<=", >);
#undef CMP
  _globals[Symbol("max")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()>=1);
    c.returnValue() = c.local(0);
    for(uintptr_t i(1); i<c.localCount(); i++)
      if(c.local(i)>c.returnValue())
        c.returnValue() = c.local(i);
  });
  _globals[Symbol("min")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()>=1);
    c.returnValue() = c.local(0);
    for(uintptr_t i(1); i<c.localCount(); i++)
      if(c.local(i)<c.returnValue())
        c.returnValue() = c.local(i);
  });
  _globals[Symbol("clamp")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==3);
    c.returnValue() = clamp(c.local(0), c.local(1), c.local(2));
  });
#define SETOP(op)\
  _globals[Symbol(#op)] = (Native)([](Context& c,const Array<Var>& a) {\
    L_ASSERT(a.size()>1);\
    Var* target(c.reference(a[1]));\
    for(uintptr_t i(2);i<a.size();i++){\
      *target op c.executeRef(a[i]);\
      c._stack.pop();\
    }\
  })
  SETOP(+= );
  SETOP(-= );
  SETOP(*= );
  SETOP(/= );
  SETOP(%= );
#undef SETOP
  _globals[Symbol("+")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()>=1);
    c.returnValue() = c.local(0);
    for(uintptr_t i(1); i<c.localCount(); i++)
      c.returnValue() += c.local(i);
  });
  _globals[Symbol("*")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()>=1);
    c.returnValue() = c.local(0);
    for(uintptr_t i(1); i<c.localCount(); i++)
      c.returnValue() *= c.local(i);
  });
  _globals[Symbol("-")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()>=1);
    if(c.localCount()==1)
      c.returnValue() = Var(0.f) - c.local(0); // TODO: replace with actual neg dynamic operator
    else {
      c.returnValue() = c.local(0);
      for(uintptr_t i(1); i<c.localCount(); i++)
        c.returnValue() -= c.local(i);
    }
  });
  _globals[Symbol("/")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = c.local(0)/c.local(1);
  });
  _globals[Symbol("%")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = c.local(0)%c.local(1);
  });
  _globals[Symbol("print")] = (Function)([](Context& c) {
    for(uintptr_t i(0); i<c.localCount(); i++)
      out << c.local(i);
  });
  _globals[Symbol("break")] = (Function)([](Context& c) {
    debugbreak();
  });
  _globals[Symbol("typename")] = (Function)([](Context& c) {
    c.returnValue() = c.local(0).type()->name;
  });
  _globals[Symbol("object")] = (Function)object;
  _globals[Symbol("now")] = (Function)([](Context& c) {
    c.returnValue() = Time::now();
  });
  _globals[Symbol("time")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = Time(c.local(0).get<float>()*1000000.f);
  });
  _globals["rand"] = (Function)([](Context& c) {
    c.returnValue() = Rand::nextFloat();
  });
  _globals[Symbol("button-pressed")] = (Function)([](Context& c) {
    if(c.localCount()) {
      if(c.local(0).is<Symbol>())
        c.returnValue() = Window::isPressed(Window::symbolToButton(c.local(0).as<Symbol>()));
      else if(c.local(0).is<int>())
        c.returnValue() = Window::isPressed((Window::Event::Button)(c.local(0).as<int>()+'0'));
    } else c.returnValue() = false;
  });
  _globals[Symbol("window-height")] = (Function)([](Context& c) { c.returnValue() = float(Window::height()); });
  _globals[Symbol("window-width")] = (Function)([](Context& c) { c.returnValue() = float(Window::width()); });
  _globals[Symbol("mouse-x")] = (Function)([](Context& c) { c.returnValue() = float(Window::mousePosition().x()); });
  _globals[Symbol("mouse-y")] = (Function)([](Context& c) { c.returnValue() = float(Window::mousePosition().y()); });
  _globals[Symbol("vec")] = (Function)([](Context& c) {
    const uint32_t local_count(c.localCount());
    Vector3f& vector(c.returnValue().make<Vector3f>());
    if(local_count)
      for(uint32_t i(0); i<3; i++)
        vector[i] = c.local(min(local_count-1, i));
    else vector = 0.f;
  });
  _globals[Symbol("vec4")] = (Function)([](Context& c) {
    const uint32_t local_count(c.localCount());
    Vector4f& vector(c.returnValue().make<Vector4f>());
    if(local_count)
      for(uint32_t i(0); i<4; i++)
        vector[i] = c.local(min(local_count-1, i));
    else vector = 0.f;
  });
  _globals[Symbol("normalize")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = c.local(0).get<Vector3f>().normalized();
  });
  _globals[Symbol("cross")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = c.local(0).get<Vector3f>().cross(c.local(1).get<Vector3f>());
  });
  _globals[Symbol("length")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = c.local(0).get<Vector3f>().length();
  });
  _globals[Symbol("distance")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = c.local(0).get<Vector3f>().dist(c.local(1).get<Vector3f>());
  });
  _globals[Symbol("dot")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = c.local(0).get<Vector3f>().dot(c.local(1).get<Vector3f>());
  });
  _globals[Symbol("sqrt")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = sqrt(c.local(0).get<float>());
  });
  _globals[Symbol("pow")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==2);
    c.returnValue() = powf(c.local(0).get<float>(), c.local(1).get<float>());
  });
  _globals[Symbol("sin")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = sinf(c.local(0).get<float>());
  });
  _globals[Symbol("cos")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = cosf(c.local(0).get<float>());
  });
  _globals[Symbol("tan")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = tanf(c.local(0).get<float>());
  });
  _globals[Symbol("lerp")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==3);
    const float a(c.local(0).get<float>()), b(c.local(1).get<float>()), alpha(c.local(2).get<float>());
    c.returnValue() = (a*(1.f-alpha)+b*alpha);
  });
  _globals[Symbol("abs")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==1);
    c.returnValue() = abs(c.local(0).get<float>());
  });
  _globals[Symbol("floor")] = (Function)([](Context& c) {
	  L_ASSERT(c.localCount()==1);
	  c.returnValue() = floorf(c.local(0).get<float>());
  });
  _globals[Symbol("color")] = (Function)([](Context& c) {
    if(c.local(0).is<String>()) {
      c.returnValue() = Color(c.local(0).as<String>());
    } else {
      Color& color(c.returnValue().make<Color>() = Color::white);
      const uint32_t params(min(c.localCount(), 4u));
      for(uint32_t i(0); i<params; i++)
        color[i] = c.local(i).is<float>() ? (c.local(i).as<float>()*255) : c.local(i).get<int>();
    }
  });
  _globals[Symbol("left-pad")] = (Function)([](Context& c) {
    L_ASSERT(c.localCount()==3);
    c.returnValue() = c.local(0).get<String>();
    String& str(c.returnValue().as<String>());
    const uint32_t wanted_size(c.local(1).get<float>());
    const String append(c.local(2).get<String>());
    while(str.size()<wanted_size) {
      str = append + str;
    }
  });
#define L_SCRIPT_ACCESS_METHOD(type,name) typeValue(Type<type>::description(), Symbol(#name)) = (Script::Function)([](Script::Context& c) {L_ASSERT(c.localCount()==0 && c.currentSelf().is<type>()); c.returnValue() = c.currentSelf().as<type>().name();})
  L_SCRIPT_ACCESS_METHOD(Vector3f, x);
  L_SCRIPT_ACCESS_METHOD(Vector3f, y);
  L_SCRIPT_ACCESS_METHOD(Vector3f, z);
}
