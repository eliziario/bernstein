# Bernstein Reasoner

This Dune package is reserved for the pure architecture and policy reasoner. It will parse and type-check the Bernstein intent language, evaluate a closed snapshot of facts and approved decisions, and return decisions, obligations, and explanation references.

It has no filesystem, network, language-server, model, credential, or process-execution authority. The C++ host owns all effects.

The reasoner does not implement an executable yet. Its public boundary is described in [the protocol README](../protocol/README.md) and RFC 0001.
