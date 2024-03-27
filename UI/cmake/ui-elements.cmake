add_library(obs-ui-support INTERFACE)
add_library(OBS::ui-support ALIAS obs-ui-support)

target_sources(
  obs-ui-support
  INTERFACE # cmake-format: sortable
            clickable-label.hpp
            double-slider.cpp
            double-slider.hpp
            horizontal-scroll-area.cpp
            horizontal-scroll-area.hpp
            plain-text-edit.cpp
            plain-text-edit.hpp
            properties-view.cpp
            properties-view.hpp
            properties-view.moc.hpp
            qt-wrappers.cpp
            qt-wrappers.hpp
            slider-ignorewheel.cpp
            slider-ignorewheel.hpp
            spinbox-ignorewheel.cpp
            spinbox-ignorewheel.hpp
            vertical-scroll-area.cpp
            vertical-scroll-area.hpp)

target_include_directories(obs-ui-support INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(obs-ui-support INTERFACE $<$<PLATFORM_ID:Linux>:-Wno-error=enum-conversion>)

target_link_libraries(obs-studio PRIVATE OBS::ui-support)

target_sources(
  obs-studio
  PRIVATE # cmake-format: sortable
          adv-audio-control.cpp
          adv-audio-control.hpp
          audio-encoders.cpp
          audio-encoders.hpp
          balance-slider.hpp
          context-bar-controls.cpp
          context-bar-controls.hpp
          focus-list.cpp
          focus-list.hpp
          hotkey-edit.cpp
          hotkey-edit.hpp
          item-widget-helpers.cpp
          item-widget-helpers.hpp
          lineedit-autoresize.cpp
          lineedit-autoresize.hpp
          log-viewer.cpp
          log-viewer.hpp
          media-controls.cpp
          media-controls.hpp
          media-slider.cpp
          media-slider.hpp
          menu-button.cpp
          menu-button.hpp
          mute-checkbox.hpp
          record-button.cpp
          record-button.hpp
          remote-text.cpp
          remote-text.hpp
          scene-tree.cpp
          scene-tree.hpp
          screenshot-obj.hpp
          slider-absoluteset-style.cpp
          slider-absoluteset-style.hpp
          source-label.cpp
          source-label.hpp
          source-tree.cpp
          source-tree.hpp
          undo-stack-obs.cpp
          undo-stack-obs.hpp
          url-push-button.cpp
          url-push-button.hpp
          visibility-item-widget.cpp
          visibility-item-widget.hpp
          volume-control.cpp
          volume-control.hpp)
