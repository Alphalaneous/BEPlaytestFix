#include "includes.h"
#include <thread>
#include <random>

#define public_cast(value, member) [](auto* v) { \
	class FriendClass__; \
	using T = std::remove_pointer<decltype(v)>::type; \
	class FriendeeClass__: public T { \
	protected: \
		friend FriendClass__; \
	}; \
	class FriendClass__ { \
	public: \
		auto& get(FriendeeClass__* v) { return v->member; } \
	} c; \
	return c.get(reinterpret_cast<FriendeeClass__*>(v)); \
}(value)

template <typename T, typename U>
T union_cast(U value) {
    union {
        T a;
        U b;
    } u;
    u.b = value;
    return u.a;
}

std::string get_module_name(HMODULE mod) {
    char buffer[MAX_PATH];
    if (!mod || !GetModuleFileNameA(mod, buffer, MAX_PATH))
        return "Unknown";
    return std::filesystem::path(buffer).filename().string();
}

int getAddr(void* addr) {
    HMODULE mod;

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<char*>(addr), &mod))
        mod = NULL;
    unsigned int x;
    std::stringstream stream;
    stream << std::hex << reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(mod));
    stream >> x;

    return static_cast<int>(x);
}

std::string getDllName(void* addr) {
    HMODULE mod;

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<char*>(addr), &mod))
        mod = NULL;

    std::stringstream stream;
    stream << get_module_name(mod);
    return stream.str();
}

bool update = true;

bool ignoreUpdateCheck = true;


void(__thiscall* LevelEditorLayer_updateEditorMode)(gd::LevelEditorLayer*);

void __fastcall LevelEditorLayer_updateEditorMode_H(gd::LevelEditorLayer* self, void*) {

    if (update) LevelEditorLayer_updateEditorMode(self);
}

void(__thiscall* LevelEditorLayer_init)(gd::LevelEditorLayer*, gd::GJGameLevel*);

void __fastcall LevelEditorLayer_init_H(gd::LevelEditorLayer* self, void*, gd::GJGameLevel* level) {

    LevelEditorLayer_init(self, level);

    LevelEditorLayer_updateEditorMode(self);

}


void(__thiscall* CCMenuItemToggler_toggle)(gd::CCMenuItemToggler*, bool on);

void __fastcall CCMenuItemToggler_toggle_H(gd::CCMenuItemToggler* self, void*, bool on) {


    const auto selector = public_cast(self, m_pfnSelector);
    const auto name = getDllName(union_cast<void*>(selector));
    const auto addr = getAddr(union_cast<void*>(selector));
    update = true;

    if (name == "BetterEdit-v4.0.5.dll") {
        if (addr == 0x0034f4) {
            std::cout << ignoreUpdateCheck << std::endl;

            update = false;
            gd::EditorUI* editorUI = gd::EditorUI::get();
            gd::LevelEditorLayer* editorLayer = gd::LevelEditorLayer::get();
            if (editorUI) { 
                CCObject* obj{};
                editorLayer->resetMovingObjects();
                editorUI->onStopPlaytest(obj);
                LevelEditorLayer_updateEditorMode(editorLayer);
            }
        }
    }

    CCMenuItemToggler_toggle(self, on);
}



DWORD WINAPI thread_func(void* hModule) {
    MH_Initialize();

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(100, 2000);

    int random = distr(gen);

    std::this_thread::sleep_for(std::chrono::milliseconds(random));

    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x15ee00),
        LevelEditorLayer_init_H,
        reinterpret_cast<void**>(&LevelEditorLayer_init)
    );

    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x1652b0),
        LevelEditorLayer_updateEditorMode_H,
        reinterpret_cast<void**>(&LevelEditorLayer_updateEditorMode)
    );
    
    MH_CreateHook(
        reinterpret_cast<void*>(gd::base + 0x199B0),
        CCMenuItemToggler_toggle_H,
        reinterpret_cast<void**>(&CCMenuItemToggler_toggle)
    );
    
    MH_EnableHook(MH_ALL_HOOKS);
    
    return 0;
}



BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, thread_func, handle, 0, 0);
        
    }
    return TRUE;
}