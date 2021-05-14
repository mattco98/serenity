set(SOURCES
    DemoList.cpp
    PathClipperWidget.cpp
    GridWidget.cpp
    main.cpp
)

serenity_app(PathClipper ICON app-libgfx-demo)
target_link_libraries(PathClipper LibGUI)
