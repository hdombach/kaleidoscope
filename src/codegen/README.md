# Overview

This code provides tools for codegenerating with an interface based on the
python library [jinja](https://jinja.palletsprojects.com/en/stable/templates/).
There is already a [cpp library](https://github.com/jinja2cpp/Jinja2Cpp) for this,
but I wanted to experiment with the parsing behind the scenes.

The primary interface is `Templgen` and examples can be seen in `TemplGenTest.cpp`
The current features are:
- if statements
- for statements
- Functions and builtin methods for types
- Expressions

Internally, I made several components that all work together.

# CfgContext

Used for describing a context free grammar. It consists of a dictionary of 
`CfgNodes` which can be constructed using overloaded operators. Some of the grammar
rules like closures you can use the `CfgContext` to build.

I ran into a bunch of problems when trying to design the cpp inteface. Originally,
I did not have a `CfgContext` but instead stored CfgNodes on the stack.

```cpp
auto first = CFG("first");
auto second = CFG("second");
auto third = CFG("third");

auto test1 = first + second | third;
```

However, I ran into a ton of issues. For example, it was difficult to reference
nodes that had not been created yet which would be helpful for recursive definitions.
Also, it would be difficult copy all the nodes to a different location.

To address this I made CfgContext to automatically store nodes and handel nodes
that have yet to be declared. The main downside is that referencing new nodes
is a little more verbose. Also, I use tons of references internally with the idea
that I'll be able to optimize them out in the future if necessary.

# SParser
A simple parser for the context free grammar. It niavely parses the nodes as is
and as a  result can potentially do a lot of unessary work when parsing. The hope
is that I can swap the parser out for something more efficient in the future.
The output is an abstract syntax tree. Which can almost be directly used
for generating final code.

# TemplObj
The objects that are used by the codegen script. Since jinja is based on python,
the objects somewhat resembly python objects. They can either be a primitive,
list, dictionary, or a function. I have some nice cpp syntax like operator overloads
and a wrapper for function objects.

# TemplGen

Generates code from AST
