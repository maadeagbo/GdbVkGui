-- Main app module. Should be skeleton that stores data and calls through to
-- modules that save no state

local ImGui     = ImGuiLib
local GuiRender = require "GuiRender"

GdbApp = {
	open_exe = "",
	exe_filename = "",
	open_file = { short = "", full = "" },

	force_reload = nil,
	module_list = {
		{ 1, "GuiRender" },
	},

	output_txt = "",
	local_vars_txt = "",
	frame_info_txt = "",

	open_dialog = false,
	open_dialog_exe = false,
	reload_module = false,
}

print("Starting GdbApp")

function GdbApp:Init(args)
	-- Increase aggressiveness GC (wait for memory to grow to 1.5x then collect)
	collectgarbage("setpause", 150)
end

function GdbApp:Update(args)
	if ImGui.BeginMainMenuBar() then
		if ImGui.BeginMenu("File") then
			if ImGui.MenuItem("ShowDemo", false) then
			end
			if ImGui.MenuItem("Exe Open", false) then
				self.open_dialog_exe = true
			end
			if ImGui.MenuItem("File Open", false) then
				self.open_dialog = true
			end
			if ImGui.BeginMenu("Text Editor Theme") then
				if ImGui.MenuItem("Dark", false) then
					SetEditorTheme(1);
				end
				if ImGui.MenuItem("Light", false) then
					SetEditorTheme(1);
				end
				if ImGui.MenuItem("RetroBlue", false) then
					SetEditorTheme(1);
				end
				ImGui.EndMenu()
			end

			ImGui.Separator()
			if ImGui.MenuItem("Reload Module", false) then
				self.reload_module = true
			end
			ImGui.EndMenu()
		end
		ImGui.EndMainMenuBar()
	end

	GuiRender:Present(GdbApp)

	-- module reloading
	
	if self.force_reload then
		local idx <const> = self.force_reload
		self.force_reload = nil

		print("Attempting to reload: \""..self.module_list[idx][2].."\"")
		package.loaded[self.module_list[idx][2]] = nil

		-- TODO : figure out better scheme
		if self.module_list[idx][1] then
			GuiRender = require(self.module_list[idx][2])
		end
	end
end