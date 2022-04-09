serenity_component(
    PathClipDemo
    TARGETS PathClipDemo
)

set(SOURCES
    DemoList.cpp
    PathClipWidget.cpp
    GridWidget.cpp
    main.cpp
    )

serenity_app(PathClipDemo ICON app-libgfx-demo)
target_link_libraries(PathClipDemo LibGUI LibGfx)
