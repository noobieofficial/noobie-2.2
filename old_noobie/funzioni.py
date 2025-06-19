import sys 
import re
import random
import traceback

SUPPORTED_TYPES = {"INT", "FLOAT", "CHAR", "STR", "BOOL"}
SUPPORTED_COMMAND = {"SET", "SAY", "HEAR", "CONVERT", 'CHANGE', 'RANDOM', 'EXIT'}
BOOLEAN_VALUE = {"true", "false", "null"}

"""UTILITY FUNCTIONS"""
def auto_round(number, max_precision=10):
    """Dinamically round a number based on its value"""
    if number == int(number):
        return int(number)
    return round(number, len(f"{number:.{max_precision}f}".rstrip('0').split('.')[-1]))

def preprocess_expression(expression):
    """Preprocess the expression to replace specific constructs"""
    replacements = {
        'true': 'True', 'false': 'False', 'null': 'None',
        'AND': 'and', 'OR': 'or', 'NOT': 'not', 'XOR': '!=', 'xor': '!=',
    }
    for key, value in replacements.items():
        expression = expression.replace(key, value)
    return expression

def evaluate_expression(expression, variables):
    """Safely evaluate an expression"""
    try:
        expression = preprocess_expression(expression)
        used_variables = re.findall(r'\b\w+\b', expression)

        # Check that all variables have the same type
        types = {variables[var]['type'] for var in used_variables if var in variables}
        if len(types) > 1:
            raise ValueError(f"Type Error: Variables have different types: {types}")
        
        local_scope = {name: data['value'] for name, data in variables.items()}
        result = eval(expression, local_scope)

        if isinstance(result, bool):
            return "true" if result else "false" 
        return auto_round(result)

    except Exception as e:
        raise ValueError(f"Calculation Error: {e}")
    
def replace_variables(line, variables):
    """Replace variable references with their values or types."""
    def subsitute(match):
        prefix, var_name = match.groups()
        if var_name in variables:
            value = variables[var_name]['value']
            type = variables[var_name]['type']

            if prefix == "@":
                if type == "BOOL":
                    return "null" if value is None else "true" if value else "false"
                if type in {"INT", "FLOAT"}:
                    return str(auto_round(value))  # Assicurati che sia una stringa
                return str(value) if value is not None else "null"
            elif prefix == '?':
                return type
        return match.group(0)
    
    return re.sub(r'(@|\?)(\w+)', subsitute, line)


"""FILE HANDLING"""
def read_code_from_file(filename):
    """Read the code from a file"""
    try:
        with open(filename, 'r', encoding='utf-8') as file:
            return file.read()
    except FileNotFoundError:
        raise ValueError(f"File Error: File '{filename}' not found")  
    except Exception as e:
        raise ValueError(f"File Error: {e}")  
    
"""ERROR HANDLING"""
def handle_error(message, line_number=None):
    if line_number:
        print(f"Error on line {line_number}: {message}")
    else:
        print(f"Error: {message}")
    if sys.exc_info()[0] is not None:
        traceback.print_exc()
    sys.exit(1)

"""VATIBALES INITIALIZATION"""
def initialize_variable(var_type, raw_value):
    """Initializes a variable based on its type."""
    var_type = var_type.upper()

    # Se il valore è None, inizializziamo la variabile con un valore di default
    if raw_value is None:
        if var_type == "INT":
            return 0
        elif var_type == "FLOAT":
            return 0.0
        elif var_type == "BOOL":
            return None
        elif var_type == "CHAR":
            return '\x00'  # carattere NULL
        elif var_type == "STR":
            return ''
        else:
            raise ValueError(f"Unsupported type: {var_type}")
        
    raw_value = str(raw_value).strip()

    # Se il valore contiene un'espressione matematica, la valutiamo
    try:
        if any(op in raw_value for op in "+-*/()"):
            raw_value = eval(raw_value)  # Valuta l'espressione in modo sicuro

        if var_type == "INT":
            return int(raw_value)
        elif var_type == "FLOAT":
            return float(raw_value)
        elif var_type == "BOOL":
            return None if raw_value.lower() == "null" else raw_value.lower() == "true"
        elif var_type == "CHAR":
            if raw_value.isdigit():
                return chr(int(raw_value))
            if len(raw_value) == 3 and raw_value.startswith("'") and raw_value.endswith("'"):
                return raw_value[1]
            raise ValueError("Invalid value for CHAR.")
        elif var_type == "STR":
            return raw_value.strip('"') if raw_value else ''
        else:
            raise ValueError(f"Unsupported type: {var_type}")

    except Exception as e:
        raise ValueError(f"Initialization error: {e}")

    
"""Value Conversion"""
def convert_value(current, old_type, new_type):
    """Convert a Value between data types"""
    old_type, new_type = old_type.upper(), new_type.upper()
    if new_type not in SUPPORTED_TYPES:
        raise ValueError(f"Unsupported type: {new_type}")

    try:
        if old_type == "INT":
            return {
                "FLOAT": float(current),
                "CHAR": chr(current),
                "STR": str(current),
                "BOOL": current != 0,
            }[new_type]
        
        elif old_type == "FLOAT":
            return {
                "INT": int(current),
                "CHAR": chr(int(current)),
                "STR": str(current),
                "BOOL": int(current) != 0,
            }[new_type]
        
        elif old_type == "CHAR":
            ascii = ord(current)
            return{
                "INT": ascii,
                "FLOAT": float(ascii),
                "STR": current,
                "BOOL": 
                ascii != 0 or
                  ascii != 32 or
                    ascii != 9 or
                      ascii != 48,
            }[new_type]
        
        elif old_type == "BOOL":
            bool_val = current if current is not None else False
            return {
                "INT" : 1 if bool_val else 0,
                "FLOAT": 1.0 if bool_val else 0.0,
                "STR": "true" if bool_val else "false",
                "CHAR": '1' if bool_val else '0',
            }[new_type]
        
        elif old_type == "STR":
            if new_type == "INT":
                return len(current)
            if new_type == "FLOAT":
                return float(len(current))
            if new_type == "BOOL":
                return bool(current.strip())
            if new_type == "CHAR":
                if len(current) == 1:
                    return current
                raise ValueError("STR to CHAR conversion requires a single character")

        raise ValueError(f"Unsupported convesione: {old_type} -> {new_type}")
    
    except Exception as e:
        raise ValueError(f"Conversione error: {e}")
    
'''Random Value Generator'''
def randomize(min, max, type):
    """Generates a random value."""
    type = type.upper()
    if type not in SUPPORTED_TYPES:
        raise ValueError(f"Unsupported type: {type}")
    if min > max:
        raise ValueError("Min value cannot be grater than max value")
    
    if type == "INT":
        return random.randint(min, max)
    if type == "FLOAT":
        return random.uniform(min, max)
    if type == "CHAR":
        if not (0 <= min <= max <= 127):
            raise ValueError("CHAR range must be between 0 and 127")
        return chr(random.randint(min, max))
    elif type == "BOOL":
        if min == 1 and max in {1, 2}:
            return random.choice(["true", "false"])
        elif min == 1 and max == 3:
            return random.choice(["true", "false", "null"])
        raise ValueError("Invali BOOL range")
    elif type == "STR":
        characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" \
                     "\\|!@#$%^&*()_+-=[]{};:'\",.<>/?`~"
        return ''.join(random.choice(characters) for _ in range(random.randint(min, max)))

    raise ValueError(f"Unsupported type: {type}")