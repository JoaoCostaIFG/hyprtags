{
  lib,
  hyprland,
}:
hyprlandPlugins.mkHyprlandPlugin {
  pluginName = "hyprtags";
  version = "0.1";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  meta = with lib; {
    homepage = "https://github.com/JoaoCostaIFG/hyprtags";
    description = "Hyprland version of DWM's tag system";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
