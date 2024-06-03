set(SOURCES
    GrammarContext.cpp
    GrammarParser.cpp
    main.cpp)

lagom_tool(CSSPropertiesGenerator LIBS LibMain)
target_include_directories(CSSPropertiesGenerator PRIVATE ..)