# Imports

Import another Rift source file by path. Paths are relative to the file that
contains the import, and `.rift` is added when the path has no extension.
The imported file's name is independent of its namespaces. A file can declare
multiple top-level namespaces and nested namespaces; callers use the names as
declared. An import alias renames the namespace when there is one top-level
namespace. With multiple top-level namespaces, it prefixes each one with the
alias.

```rift
using std
import "math"

func main() {
    println(math.double(21))
}
```

Use an alias to choose the namespace prefix at the import site:

```rift
using std
import calc "math"
using calc

func main() {
    println(double(21))
}
```

Imports can be nested. A source file is loaded once per program; importing the
same file under different aliases is an error. Import cycles and imported
`main` functions are also errors. The built-in standard library keeps its
existing `using std` syntax.
