from token import *
from io import BytesIO
from tokenize import tokenize, TokenError

from kumir_constants import *

SECONDARY_KWD = {"in", "as", "is", "and", "or", "not",
                 "pass", "break", "continue", "return",
                 "else", "elif",
                 "if", "except", "finally", "try",
                 "raise"}

PRIMARY_KWD = {"def", "for", "class", "import", "from",
               "with", "global", "None", "while", "yield",
               "nonlocal", "lambda", "assert",
               "del"}


class Line:
    def __init__(self):
        self.text = ""
        self.rank = (0, 0)
        self.error = None
        self.props = []
        self.tokens = []


class SyntaxHighlighter:
    def __init__(self):
        self.full_text = ""
        self.lines = []

    def set_source_text(self, text: str):
        self.full_text = text
        self.lines.clear()
        lines = text.splitlines()
        for line_text in lines:
            l = Line()
            l.text = line_text
            l.props = [LxTypeEmpty] * len(line_text)
            self.lines += [l]
        self.__process_token_analysys()

    def set_source_line_and_get_props(self, line_no: int, text: str):
        if line_no < len(self.lines):
            l = self.lines[line_no]
        else:
            l = Line()
            self.lines.append(l)
        l.text = text
        l.error = None
        l.props = [LxTypeEmpty] * len(text)
        lines = [line.text for line in self.lines]
        self.full_text = "\n".join(lines)
        self.__process_token_analysys()
        return l.props

    def get_errors(self):
        return [line.error for line in self.lines if line.error]

    def get_line_properties(self):
        return [line.props for line in self.lines]

    def get_line_ranks(self):
        return [line.rank for line in self.lines]

    def __process_token_analysys(self):
        tokens_generator = tokenize(BytesIO(self.full_text.encode('utf-8')).readline)
        start_line = 0
        start_col = 0
        try:
            for next_token in tokens_generator:
                ttype, tstring, tstart, tend, tline = next_token
                if ttype == DEDENT:
                    pass  # decrease end line rank
                elif ttype == INDENT:
                    pass  # increase line rank
                elif ttype == NEWLINE:
                    start_line += 1
                    start_col = 0
                if tstart != tend and ttype == NAME:
                    self.__process_name_token(next_token, start_line, start_col)
        except TokenError as e:
            pass


    def __process_name_token(self, token, start_line, start_col):
        _, tstring, tstart, tend, tline = token
        if tstring in PRIMARY_KWD:
            lexem_type = LxTypePrimaryKwd
        elif tstring in SECONDARY_KWD:
            lexem_type = LxTypeSecondaryKwd
        elif "True" == tstring:
            lexem_type = LxConstBoolTrue
        elif "False" == tstring:
            lexem_type = LxConstBoolFalse
        else:
            lexem_type = LxTypeName
        self.__set_lexem_type(tstart, tend, lexem_type)

    def __set_lexem_type(self, tstart, tend, lexem_type):
        line_start, col_start = tstart
        line_end, col_end = tend

        # tokenizer return line numbers starting from 1
        line_start -= 1
        line_end -= 1

        for line_no in range(line_start, line_end+1):
            l = self.lines[line_no]
            start = col_start if line_no==line_start else 0
            end = col_end if line_no==line_end else len(l.text)
            for col_no in range(start, end):
                l.props[col_no] = lexem_type
