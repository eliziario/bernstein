# Bernstein Reasoning Protocol

The C++ host and OCaml reasoner communicate through a versioned, schema-defined request/response protocol. The first transport will be selected by the Phase 0 spike; local subprocess communication is the default candidate because it isolates failures and keeps the semantic subsystem independently testable.

The protocol must carry only closed inputs and results:

~~~text
request: protocol version, policy digest, workspace snapshot, authenticated caller,
         action proposal, project facts, approved decisions

response: decision, explanation, rule/proof references, obligations, relied-on evidence
~~~

The reasoner receives no path that it can read, no credentials, and no capability to cause effects. The C++ host validates every response before using it to mint a capability.

Concrete schemas and generated bindings are intentionally deferred to the Phase 0 comparison of JSON and a binary schema system. The protocol owns the meaning of the data; the transport does not.
