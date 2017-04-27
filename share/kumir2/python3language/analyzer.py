import copy

import sys

from check_syntax.syntax_highlighting import SyntaxHighlighter, Line
from kumir_constants import *
from check_syntax.error import Error
from check_syntax.introspector import Introspector

instances = {}


from types_analyzer2 import parsers
from types_analyzer2 import special_internal_types
from types_analyzer2 import functable
from types_analyzer2 import typetable
from types_analyzer2 import basic_type_check


def __parse_base_types(types_table, methods_table):
    from types_analyzer2.headers import base_types_0 as module_0
    parsers.parse_module_classes(types_table, methods_table, module_0, "")
    types_table.append(special_internal_types.ModuleTypeDef(types_table))


def __parse_builtins(types_table, methods_table):
    from types_analyzer2.headers import builtins_0 as module_0
    from types_analyzer2.headers import builtins_1 as module_1
    parsers.parse_module_functions(types_table, methods_table, module_0, "")
    parsers.parse_module_functions(types_table, methods_table, module_1, "")


BASE_TYPES = typetable.TypesTable()
BASE_METHODS = functable.MethodsTable()
__parse_base_types(BASE_TYPES, BASE_METHODS)
__parse_builtins(BASE_TYPES, BASE_METHODS)

# noinspection PyBroadException
def import_checkers(use_pep8: bool):
    r = []
    try:
        from check_syntax import pylint_wrapper
        r += [pylint_wrapper]
    except:
        pass
    try:
        from check_syntax import pyflakes_wrapper
        r += [pyflakes_wrapper]
    except:
        pass
    if use_pep8:
        try:
            from check_syntax import pep8_wrapper
            r += [pep8_wrapper]
        except:
            pass
    return r



class AnalyzerInstance:
    next_internal_id = 0
    use_pep8 = False
    instances = {}

    def __init__(self):
        AnalyzerInstance.next_internal_id += 1
        self.internal_id = AnalyzerInstance.next_internal_id
        AnalyzerInstance.instances[self.internal_id] = self
        self.source_dir_name = ""
        self.source_text = ""
        self.errors = []
        self.symbol_table = None
        self.type_errors = []
        self.types_table = None
        self.methods_table = None
        self.syntax_highlighter = SyntaxHighlighter()
        self.introspector = Introspector()

    def set_source_dir_name(self, dir_name: str):
        self.source_dir_name = dir_name
        self.introspector.set_source_dir_name(dir_name)

    def set_source_text(self, source_text: str):
        self.source_text = source_text
        # self.types_table = copy.deepcopy(BASE_TYPES)
        # self.methods_table = copy.deepcopy(BASE_METHODS)
        # self.symbol_table, self.type_errors = basic_type_check.build_symbol_table_for_text(
        #     source_text, sys.path, "", self.types_table, self.methods_table
        # )
        self.introspector.set_source_text(source_text)
        # if self.symbol_table:
        #     self.introspector.set_symbol_table(self.symbol_table)
        self.errors.clear()

        if source_text:
            self.__perform_syntax_checks()

    def get_global_names(self):
        context = self.introspector.get_global_context()
        return {
            "modules": context.get_module_names(),
            "functions": context.get_function_names(),
            "classes": context.get_class_names()
        }


    @staticmethod
    def get_line_prop(line_no: int, text: str):
        text_length = len(text)
        fake_result = [LxTypeEmpty] * text_length
        result = fake_result
        return result

    def __perform_syntax_checks(self):
        self.syntax_highlighter.set_source_text(self.source_text)
        checkers = import_checkers(AnalyzerInstance.use_pep8)
        for checker in checkers:
            checker.set_source_text(self.source_text)
            errors = checker.get_errors()
            for error in errors:
                error.origin_name = checker.name
                if error not in self.errors:
                    self.errors += [error]


def create():
    analyzer = AnalyzerInstance()
    id = analyzer.internal_id
    return id


def remove(id: int):
    if id in AnalyzerInstance.instances:
        del instances[id]


def set_source_dir_name(id: int, dir_name: str):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    analyzer.set_source_dir_name(dir_name)


def set_source_text(id: int, text: str):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    analyzer.set_source_text(text)


def get_errors(id: int):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    sh = analyzer.syntax_highlighter
    assert isinstance(sh, SyntaxHighlighter)
    return sh.get_errors() + analyzer.errors


def get_line_properties(id: int):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    sh = analyzer.syntax_highlighter
    assert isinstance(sh, SyntaxHighlighter)
    props = sh.get_line_properties()
    for error in analyzer.errors:
        if 0 <= error.line_no <= len(props):
            props = props[error.line_no]
            start = error.start_pos
            end = start + error.length
            if end <= len(props):
                for i in range(start, end):
                    props[i] |= LxTypeError
    return props


def get_line_ranks(id: int):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    sh = analyzer.syntax_highlighter
    assert isinstance(sh, SyntaxHighlighter)
    return sh.get_line_ranks()


def get_line_prop(id: int, line_no: int, text: str):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    sh = analyzer.syntax_highlighter
    assert isinstance(sh, SyntaxHighlighter)
    return sh.set_source_line_and_get_props(line_no, text)


def get_global_names(id: int):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    return analyzer.get_global_names()


def get_syntax_highlight_hints(id: int):
    analyzer = AnalyzerInstance.instances[id]
    assert isinstance(analyzer, AnalyzerInstance)
    introspector = analyzer.introspector
    return [
        {
            "k": hint.kind,
            "l": hint.line,
            "n": hint.name
        }
        for hint in introspector.get_hints()
    ]


def set_use_pep8(use: bool):
    AnalyzerInstance.use_pep8 = use

