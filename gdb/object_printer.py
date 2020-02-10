import gdb

class ObjectPrinter:
    def __init__(self, value):
        self.value = value

    def to_string (self):
        return "?"+self.value["int_value"].string()

def lookup_type (value):
    if hasattr(value, 'type'):
        return ObjectPrinter(value)
    return None

gdb.pretty_printers.append (lookup_type)