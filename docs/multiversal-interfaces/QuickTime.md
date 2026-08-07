# QuickTime Interfaces

Source: `multiversal/defs/QuickTime.yaml`

- Functions: **21**
- Typedefs: **2**
- Structs: **1**, Unions: **0**
- Enums: **0**
- Function pointers: **0**
- Common blocks: **0**
- Dispatchers: **1**
- Low-memory globals: **0**

## Functions

### CloseMovieFile  

```c
OSErr CloseMovieFile(INTEGER refnum)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposeMovie  

```c
void DisposeMovie(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### DisposeMovieController  

```c
void DisposeMovieController(ComponentInstance cntrller)
```

Trap: — (executor 实现，无 trap) executor=C_

### EnterMovies  

```c
OSErr EnterMovies()
```

Trap: — (executor 实现，无 trap) executor=C_

### ExitMovies  

```c
void ExitMovies()
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMovieBox  

```c
void GetMovieBox(Movie movie, Rect* boxp)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMoviePreferredRate  

```c
Fixed GetMoviePreferredRate(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### GetMovieVolume  

```c
INTEGER GetMovieVolume(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### GoToBeginningOfMovie  

```c
void GoToBeginningOfMovie(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### IsMovieDone  

```c
Boolean IsMovieDone(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### MoviesTask  

```c
void MoviesTask(Movie movie, LONGINT maxmillisecs)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewMovieController  

```c
ComponentInstance NewMovieController(Movie movie, const Rect* mrectp, LONGINT flags)
```

Trap: — (executor 实现，无 trap) executor=C_

### NewMovieFromFile  

```c
OSErr NewMovieFromFile(Movie* moviep, INTEGER refnum, INTEGER* residp, StringPtr resnamep, INTEGER flags, Boolean* datarefwaschangedp)
```

Trap: — (executor 实现，无 trap) executor=C_

### OpenMovieFile  

```c
OSErr OpenMovieFile(const FSSpec* filespecp, INTEGER* refnump, uint8_t perm)
```

Trap: — (executor 实现，无 trap) executor=C_

### PrerollMovie  

```c
OSErr PrerollMovie(Movie movie, TimeValue time, Fixed rate)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetMovieActive  

```c
void SetMovieActive(Movie movie, Boolean active)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetMovieBox  

```c
void SetMovieBox(Movie movie, const Rect* boxp)
```

Trap: — (executor 实现，无 trap) executor=C_

### SetMovieGWorld  

```c
void SetMovieGWorld(Movie movie, CGrafPtr cgrafp, GDHandle gdh)
```

Trap: — (executor 实现，无 trap) executor=C_

### StartMovie  

```c
void StartMovie(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### StopMovie  

```c
void StopMovie(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

### UpdateMovie  

```c
OSErr UpdateMovie(Movie movie)
```

Trap: — (executor 实现，无 trap) executor=C_

## Typedefs

- **Movie** = MovieRecord*
- **TimeValue** = LONGINT

## Structs

- **MovieRecord** { data: LONGINT[1] }

## Dispatchers

- **QuickTime**—

