#ifndef win_menu_impl_h
#define win_menu_impl_h

#include <Windows.h>
#include <optional>
#include <queue>
#include <functional>
#include <vector>

#include "../menu/menu.h"

namespace DeskGap {
    struct MenuItem::Impl {
    	std::string role;
    	Type type;
    	UINT_PTR identifier;

    	void SetLabel(const std::string& label);

    	bool enabled;
    	bool checked;
    	void UpdateState();

    	MenuItem::EventCallbacks callbacks;

    	using Action = std::function<void(HMENU)>;
    	//pending changes before CreateMenu 
        std::queue<Action> pendingActions_;
        std::optional<HMENU> parentHMenu_;

        void AddAction(Action&& action);
        void AppendTo(HMENU parentHMenu);

    };
    
    struct Menu::Impl {
    	HMENU hmenu;
    	std::vector<std::function<void()>*> clickHandlers;

    };
}

#endif
