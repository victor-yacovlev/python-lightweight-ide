import inspect
import ast
import symtable


def get_builtins_classes_and_functions():
    classes = []
    functions = []
    all = __builtins__.items()
    for name, obj in all:
        assert isinstance(name, str)
        if not name.startswith("__"):
            if inspect.isfunction(obj) or inspect.isbuiltin(obj):
                functions += [name]
            elif inspect.isclass(obj):
                classes += [name]
    classes.remove("range")
    functions += ["range"]
    return classes, functions


class Context:
    def __init__(self, range):
        self.range = None
        self.modules = []
        self.functions = []
        self.classes = []
        self.clean_start = (0, 0, 0)
        self.children = []

    def get_module_names(self):
        return self.modules

    def get_function_names(self):
        return self.functions

    def get_class_names(self):
        return self.classes

    def clear(self):
        m, c, f = self.clean_start
        self.modules = self.modules[:m]
        self.classes = self.classes[:c]
        self.functions = self.functions[:f]
        self.children = []

class Hint:
    NO_HINT = 0
    MODULE = 1
    FUNCTION = 2
    CLASS = 3

    def __init__(self):
        self.kind = Hint.NO_HINT
        self.line = -1
        self.name = ""

class HighlightVisitor(ast.NodeVisitor):
    def __init__(self, hints: list):
        self.hints = hints

    def visit_FunctionDef(self, node):
        assert isinstance(node, ast.FunctionDef)
        func_name = node.name
        hint = Hint()
        hint.kind = Hint.FUNCTION
        hint.line = node.lineno - 1
        hint.name = func_name
        self.hints += [hint]




class Introspector:
    def __init__(self):
        self.ast = None
        self.source_text = ""
        self.source_dir_name = ""
        self.global_context = Context(None)
        glob_types, glob_funs = get_builtins_classes_and_functions()
        self.global_context.functions = glob_funs
        self.global_context.classes = glob_types
        self.global_context.clean_start = (
            len(self.global_context.modules),
            len(self.global_context.classes),
            len(self.global_context.functions)
        )
        self.symbol_table = None
        self.hints = []
        self.visitor = HighlightVisitor(self.hints)


    def set_source_dir_name(self, dir_name: str):
        self.source_dir_name = dir_name

    def set_source_text(self, text: str):
        self.source_text = text
        self.hints.clear()
        astree = None
        try:
            astree = ast.parse(text)
        except SyntaxError:
            pass
        if astree:
            self.hints.clear()
            self.visitor.visit(astree)

    def get_hints(self):
        return self.hints


    def get_global_context(self):
        return self.global_context

    def set_symbol_table(self, symbol_table: symtable.SymbolTable):
        self.symbol_table = symbol_table
        if self.symbol_table:
            self.rebuild_contexts_from_tables(self.symbol_table)

    def rebuild_contexts_from_tables(self, symbol_table: symtable.SymbolTable):
        self.global_context.clear()
        child_tables = symbol_table.get_children()
        self.build_context_from_table(
            self.global_context,
            symbol_table
        )

    def build_context_from_table(self, context: Context, symbol_table: symtable.SymbolTable):
        symbols = symbol_table.get_symbols()
        child_tables = symbol_table.get_children()
        for sym in symbols:
            name = sym.get_name()
            typedef = getattr(sym, "typedef", None)
            if typedef:
                typedef_name = typedef.name
                if "module" == typedef_name:
                    context.modules.append(name)

        for child in child_tables:
            if isinstance(child, symtable.Function):
                context.functions += [child.get_name()]
            elif isinstance(child, symtable.Class):
                context.classes += [child.get_name()]
            child_context = Context(None)
            context.children += [child_context]
            self.build_context_from_table(child_context, child)

