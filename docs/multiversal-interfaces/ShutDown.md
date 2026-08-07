# ShutDown Interfaces

Source: `multiversal/defs/ShutDown.yaml`

- Functions: **4**
- Typedefs: **0**
- Structs: **0**, Unions: **0**
- Enums: **1**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### ShutDwnInstall  

```c
void ShutDwnInstall(ProcPtr shutdown_proc, int16_t flags)
```

Trap: — (executor 实现，无 trap) executor=C_

### ShutDwnPower  

```c
void ShutDwnPower()
```

Trap: — (executor 实现，无 trap) executor=C_

### ShutDwnRemove  

```c
void ShutDwnRemove(ProcPtr shutdown_proc)
```

Trap: — (executor 实现，无 trap) executor=C_

### ShutDwnStart  

```c
void ShutDwnStart()
```

Trap: — (executor 实现，无 trap) executor=C_

## Enums

- **?**

### Enum Values

**anonymous**:

- `sdOnPowerOff` = 1
- `sdOnRestart` = 2
- `sdOnUnmount` = 4
- `sdOnDrivers` = 8
- `sdOnRestartOrPower` = sdOnPowerOff | sdOnRestart

## Dispatchers

- **ShutDown**—

