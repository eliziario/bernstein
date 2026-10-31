# The protocol between the host and the reasoner

The C++ host and the OCaml reasoner talk to each other through a versioned, schema-defined exchange of requests and responses. Nothing of it is built; what follows is the shape we intend, and the constraint that shape exists to enforce.

A request carries a closed snapshot and a response carries a decision about it:

~~~text
request: protocol version, policy digest, workspace snapshot, authenticated caller,
         action proposal, project facts, approved decisions

response: decision, explanation, rule/proof references, obligations, relied-on evidence
~~~

Read the request field by field and notice what is absent. There is no path the reasoner could open, no credential, and no handle by which it might cause something to happen. It is given facts, not the means of gathering them, which is what allows the same request to produce the same decision on a developer's machine and on a build worker. The host, for its part, validates every response before it uses one to mint a capability, because a response is data arriving from another process and is treated as such.

The transport has not been chosen. A local subprocess is the current favourite, on the grounds that it isolates a crash and lets the reasoner be tested on its own, but the choice should rest on evidence about packaging, restarts, and debugging rather than on an argument about the cost of a call. The concrete schemas and the generated bindings are likewise deferred until we have compared JSON with a binary schema system. What is not deferred is the division of labour: the protocol fixes what the data means, and the transport is only how it travels.

[RFC 0001](../docs/rfc-0001-core-interfaces.md) describes the surrounding boundary — where a proposal comes from, what a decision is permitted to authorize, and what the host does with one afterwards.
