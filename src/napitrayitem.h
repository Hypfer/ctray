#ifndef _NAPITRAYITEM_H
#define _NAPITRAYITEM_H

#include <napi.h>
#include <stdlib.h>

#define THROW(error) Napi::TypeError::New( env, error ).ThrowAsJavaScriptException()

#define EMIT(event_name, ...) info.This().As<Napi::Object>().Get("emit").As<Napi::Function>().Call(info.This(), {Napi::String::New(info.Env(), event_name), __VA_ARGS__})

class NapiTrayItem : public Napi::ObjectWrap<NapiTrayItem> {
    public:

        static void OnClickCallBack(Napi::Env env, Napi::Function emitter, NapiTrayItem *data){
            emitter.Call(data->Value(), {Napi::String::New(env, "click")});
        }

        static Napi::Object Init(Napi::Env env, Napi::Object exports){
            Napi::Function func =
                DefineClass(env, "TrayItem", {
                    InstanceMethod("click", &NapiTrayItem::Click),
                    InstanceAccessor("text", &NapiTrayItem::GetText, &NapiTrayItem::SetText),
                    InstanceAccessor("checked", &NapiTrayItem::GetChecked, &NapiTrayItem::SetChecked),
                    InstanceAccessor("disabled", &NapiTrayItem::GetDisabled, &NapiTrayItem::SetDisabled),
                    InstanceAccessor("callback", &NapiTrayItem::GetCallback, &NapiTrayItem::SetCallback),
                    InstanceAccessor("submenu", &NapiTrayItem::GetSubmenu, &NapiTrayItem::SetSubmenu)});
            
            // Tray extends EventEmitter
            Napi::Function Events = env.Global().As<Napi::Object>().Get("require").As<Napi::Function>().Call(env.Global(), {Napi::String::New(env, "events")}).As<Napi::Function>();
            Napi::Function SetPrototype = env.Global().Get("Object").As<Napi::Object>().Get("setPrototypeOf").As<Napi::Function>();
            
            SetPrototype.Call(env.Global(), {func.Get("prototype"), Events.Get("prototype")});

            Napi::FunctionReference *constructor = new Napi::FunctionReference(); 
            *constructor = Napi::Persistent(func);

            return func;
        }

        NapiTrayItem(const Napi::CallbackInfo& info, const Napi::Value& arg) : Napi::ObjectWrap<NapiTrayItem>(info) {
            Napi::Env env = info.Env();
            
            Napi::Object obj = info[0].As<Napi::Object>();
            if( info[0].IsString() ){
                obj = Napi::Object::New(env);
                obj.Set("text", info[0].As<Napi::String>());
            }
            else if( !info[0].IsObject() ){
                THROW("use new MenuItem(string) or new MenuItem(object)");
                return;
            }
            Napi::Function Emit = Value().Get("emit").As<Napi::Function>();
            onClickCallback = Napi::ThreadSafeFunction::New(env, Emit, "emit-click", 0, 1);
            
            SetText(info, obj.Get("text"));
            SetChecked(info, obj.Get("checked"));
            SetDisabled(info, obj.Get("disabled"));
            SetSubmenu(info, obj.Get("submenu"));
            SetCallback(info, obj.Get("callback"));
        }

        NapiTrayItem(const Napi::CallbackInfo& info) : NapiTrayItem(info, info[0]) {}

        Napi::Value GetText (const Napi::CallbackInfo& info){
            return Napi::String::New(info.Env(), text);
        }
        void SetText(const Napi::CallbackInfo& info, const Napi::Value& arg){
            Napi::Env env = info.Env();
            if( !arg.IsString() ){
                THROW("'text' property must be a string");
                return;
            }
            text = arg.As<Napi::String>().Utf8Value();
            EMIT("update", Napi::String::New(env, "text"));
        }
        Napi::Value GetChecked (const Napi::CallbackInfo& info){
            return Napi::Boolean::New(info.Env(), checked);
        }
        void SetChecked(const Napi::CallbackInfo& info, const Napi::Value& arg){
            if( arg.IsUndefined() || arg.IsNull() ){
                checked = false;
                return;
            }
            checked = arg.ToBoolean();
            EMIT("update", Napi::String::New(info.Env(), "checked"));
        }
        Napi::Value GetDisabled (const Napi::CallbackInfo& info){
            return Napi::Boolean::New(info.Env(), disabled);
        }
        void SetDisabled(const Napi::CallbackInfo& info, const Napi::Value& arg){
            if( arg.IsUndefined() || arg.IsNull() ){
                disabled = Napi::Boolean::New(info.Env(), false);
                return;
            }
            disabled = arg.ToBoolean();
            EMIT("update", Napi::String::New(info.Env(), "disabled"));
        }
        Napi::Value GetCallback (const Napi::CallbackInfo& info){
            return callback.Value();
        }
        void SetCallback(const Napi::CallbackInfo& info, const Napi::Value& arg){
            Napi::Object This = info.This().As<Napi::Object>();
            Napi::Function on = This.Get("on").As<Napi::Function>();
            Napi::Function off = This.Get("off").As<Napi::Function>();
            
            if( !callback.IsEmpty() ){
                off.Call(info.This(), {Napi::String::New(info.Env(), "click"), callback.Value()});
            }

            if( !arg.IsFunction() ){
                Napi::Function f = Napi::Function();
                callback = Napi::Persistent(f);
            }
            else{
                callback = Napi::Persistent(arg.As<Napi::Function>());
                on.Call(info.This(), {Napi::String::New(info.Env(), "click"), callback.Value()});
            }
        }
        Napi::Value GetSubmenu(const Napi::CallbackInfo& info){
            return submenupointer.Value();
        }
        void SetSubmenu(const Napi::CallbackInfo& info, const Napi::Value& arg){
            Napi::Array submenu;
            if( !arg.IsArray() ){
                submenu = Napi::Array::New(info.Env(), 0);
            }
            else{
                Napi::Array array = arg.As<Napi::Array>();
                Napi::Function constructor = info.Env().GetInstanceData<Napi::FunctionReference>()->Value().Get("MenuItem").As<Napi::Function>();
                submenu = Napi::Array::New(info.Env(), array.Length());
                for(uint32_t i = 0; i < array.Length(); i++){
                    Napi::Value arg = array.Get(i);
                    if( arg.IsObject() && arg.As<Napi::Object>().InstanceOf(constructor) ){
                        submenu[i] = arg;
                    }
                    else{
                        submenu[i] = constructor.New({ array.Get(i) });
                    }
                }
            }
            submenupointer = Napi::Persistent(submenu);
            EMIT("update", Napi::String::New(info.Env(), "submenu"));
        }

        void Click(const Napi::CallbackInfo& info){
            EMIT("click", NULL);
        }

        void Click(){
            onClickCallback.BlockingCall( this, OnClickCallBack);
        }

        void* PrepareItem(void *origin);

        void Destroy(){
            onClickCallback.Release();
        }
        
        Napi::ThreadSafeFunction onClickCallback;
        std::string text;
        bool checked;
        bool disabled;
        Napi::FunctionReference callback;
        Napi::Reference<Napi::Array> submenupointer;
        void* itemPointer = nullptr;
};

#endif