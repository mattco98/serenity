set(SOURCES
    main.cpp)

lagom_tool(CSSPropertiesGenerator LIBS LibMain)
target_include_directories(CSSPropertiesGenerator PRIVATE ..)