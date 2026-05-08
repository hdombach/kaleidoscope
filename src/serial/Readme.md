# Serial

A library automatically handling serialization and undo/redo capability.

There will be two major categories of data in the running application:
persistent and temporary data. Persistent data is any information about that
scene, nodes, cameras, ect that should be saved. The temporary data includes
all information like gui state. The goal is to have all the persistent data
stored in the serialized data structure. This will probably necessitate
wrapper classes to interface withh specific parts of the program.

## Serialization
The serialization is very inspired by protobuf. But, it will also attempt to
add logic for safely migrating between versions.

The structure of the data will be described by a separate file containing what
looks like C structs with extra syntax for containers. Like protobufs, this
file will then be used to generate source code. A set of getters and setters
will be generated for every field as well as a `serialize` and `deserialize`
function.

## Versioning
The serialization syntax will also contain ways of defining multiple versions
at once. This allows you to safely migrate versions.

The serialization library uses the following version numbering scheme.
```
Vanity.Major.Minor
```
- The vanity field can be used when you want to represent a significant change
  to the user.
- The major field is used for breaking changes
- The minor field is used for non-breaking changes.

Non-breaking changes include any variations that can still be safely opened by
different versions of the program. At the moment, the only valid non-breaking
change is appending a field to the end of a struct. This extra field is marked
as optional and will automatically be left empty when loading other versions.

For breaking changes, a completely separate data structure is generated. A
function skeleton will be generated for transforming from an old version to a
new version, but it is up to the developer to implement the logic that does the
transformation. However, only incremental changes need to be implemented. The
library can automatically chain together multiple incremental changes for you
allowing you to migrate between any version.

## Undo / Redo
The last major feature that this library provides is undo and redo capability.
The main benefit of codegenerating the interface is that you automatically
generate boiler code for things like serialization and undo / redo
transactions. Moreover, the unified interface ensures you can easily support
undo / redo for any data in the program with hopefully no significant developer
cost for future additions.

While more complex systems could be implimented in the future, I will start
with a system based around a double-sided queue that holds transactions. Any
change you make appends a transaction to the front of the queue. A transaction
includes the primitive that was changed as well as the location in the parent
containers recursively. In order to undo and redo, you move a pointer backward
and forward starting from the front queue and apply the corresponding change
every step along the way. As soon as you make a change (a new transaction), you
delete everything after the pointer. When the queue starts getting to long, you
delete from the back of the queue to make space.

### Restrictions
- Most primitive changes in the transaction can be represented by a brand new
  value. For example, if you change an integer value, it is space efficient to
  just store the old number in its entirety. However, some primitives like
  strings might need to have smarter diffs. For example, a material shader
  could be potentially stored as a source code string. If it gets too big, it
  might be necessary to implement a way to store and apply string diffs.
- Any data structures like lists that are used will need to be implemented in
  such a way that you can reverse any operation.

### Implementation notes
The generated classes will contain a separate getter and setter for every field
that is defined. Each setter will include logic for automatically creating a
transaction whenever a field is updated. This allows undo functionality to be
implemented invisible.

In some situations, it makes sense to expose the creation of transactions to
the user. One situation that might occur is if there is an operation in the gui
that results in multiple fields being updated at once. When applying undo, it
is important that the multiple changes be applied at once or else it might
result in unexpected behavior. In order to fix this, a set of functions
`start_transaction` and `stop_transaction` will be provided in the interface.
When you call `start_transaction`, the seria class will start queueing up
changes until `stop_transaction` is called. These changes will then be bundled
together as a single transaction.

A second situation that might occur is if a single change takes place over
multiple refreshes. For example, I will be implementing a node editor. I do not
want to update the position of a node every frame as it is being dragged. This
would make the undo history completely unusable. Instead, I want to start a
transaction on mouse down and stop the transaction on mouse up. This means, the
data structure needs to correctly handle a temporary state where the values get
added but the double-sided queue isn't updated immediately. This behavior can
probably still be provided by `start_transaction` and `stop_transaction`. It
just means that `stop_transaction` can potentially be called multiple frames
after `start_transaction`.

Neither `start_transaction` or `stop_transaction` can be called two times in a
row. The only valid behavior is to swap between a recording an ready state.

If you call a set function without first calling `start_transaction`, a
transaction will automatically be started and stopped for you creating a
transaction with the single change.

You can only call undo / redo while the class is in a ready state. This
might cause edge cases if you try to undo while doing an action. (Pressing
cmd-z while dragging a node).
