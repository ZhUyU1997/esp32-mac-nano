# DiskInit Interfaces

Source: `multiversal/defs/DiskInit.yaml`

- Functions: **6**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### DIBadMount  

```c
INTEGER DIBadMount(Point pt, LONGINT evtmess)
```

Trap: — (executor 实现，无 trap) executor=C_

### DIFormat  

```c
OSErr DIFormat(INTEGER dn)
```

Trap: — (executor 实现，无 trap) executor=C_

### DILoad  

```c
void DILoad()
```

Trap: — (executor 实现，无 trap) executor=C_

### DIUnload  

```c
void DIUnload()
```

Trap: — (executor 实现，无 trap) executor=C_

### DIVerify  

```c
OSErr DIVerify(INTEGER dn)
```

Trap: — (executor 实现，无 trap) executor=C_

### DIZero  

```c
OSErr DIZero(INTEGER dn, StringPtr vname)
```

Trap: — (executor 实现，无 trap) executor=C_

## Dispatchers

- **Pack2**—

