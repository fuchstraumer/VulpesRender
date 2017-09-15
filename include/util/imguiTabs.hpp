#pragma once
#include "vpr_stdafx.h"
#include "../../imgui/imgui.h"

namespace ImGui {
	/*
	tabLabels: name of all tabs involved
	tabSize: number of elements
	tabIndex: holds the current active tab
	tabOrder: optional array of integers from 0 to tabSize-1 that maps the tab label order. If one of the numbers is replaced by -1 the tab label is not visible (closed). It can be read/modified at runtime.

	// USAGE EXAMPLE
	static const char* tabNames[] = {"First tab","Second tab","Third tab"};
	static int tabOrder[] = {0,1,2};
	static int tabSelected = 0;
	const bool tabChanged = ImGui::TabLabels(tabNames,sizeof(tabNames)/sizeof(tabNames[0]),tabSelected,tabOrder);
	ImGui::Text("\nTab Page For Tab: \"%s\" here.\n",tabNames[tabSelected]);
	*/

	IMGUI_API bool TabLabels(const char **tabLabels, int tabSize, int &tabIndex, int *tabOrder = NULL);

} // namespace ImGui