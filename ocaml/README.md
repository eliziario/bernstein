# The reasoner

This Dune package is where the architecture and policy reasoner will live. Its job is to parse and type-check the language in which constraints are written, evaluate a closed snapshot of facts and approved decisions against them, and return a decision together with its obligations and references to the rules that produced it.

It is also defined by what it is denied. The reasoner has no access to the filesystem, the network, a language server, a model, a credential, or a process, and this is enforced by what the package is allowed to depend on rather than by convention. Everything effectful belongs to the C++ host. The reason for drawing the line in this place is practical: a component that cannot touch anything can be tested exhaustively on its inputs, and a decision it reaches today can be reproduced tomorrow.

There is no executable yet — the package currently holds its build definition and nothing else. The boundary it has to implement is described in [the protocol](../protocol/README.md) and, in more detail, in [RFC 0001](../docs/rfc-0001-core-interfaces.md).
