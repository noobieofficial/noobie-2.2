import sys
import re
from typing import Dict, List, Optional, Callable
from func import *

class NoobieInterpreter:
    """Main interpreter class for the Noobie language"""
    
    def __init__(self):
        self.variables: Dict[str, Variable] = {}
        self.in_comment_block = False
        self.command_handlers = self._initialize_command_handlers()
    
    def _initialize_command_handlers(self) -> Dict[str, Callable]:
        """Initialize command handlers for better maintainability"""
        return {
            'exit': self._handle_exit,
            'say': self._handle_say,
            'set': self._handle_set,
            'hear': self._handle_hear,
            'change': self._handle_change,
            'convert': self._handle_convert,
            'random': self._handle_random,
            'round': self._handle_round,
            'del': self._handle_del,
            'reset': self._handle_reset,
            'increment': self._handle_increment,
            'decrement': self._handle_decrement,
            'swap': self._handle_swap,
            'uppercase': self._handle_uppercase,
            'lowercase': self._handle_lowercase,
            'reverse': self._handle_reverse,
        }
    
    def _evaluate_expression_with_parentheses(self, expression: str) -> Any:
        """Evaluate expression with better error handling"""
        try:
            return evaluate_expression(expression, self.variables)
        except NoobieError:
            raise
        except Exception as e:
            raise NoobieError(f"Error evaluating expression: {e}")
    
    def _extract_expression(self, message: str) -> str:
        """Extract and execute expressions in curly braces"""
        pattern = r'\{([^{}]+)\}'
        
        while re.search(pattern, message):
            match = re.search(pattern, message)
            if not match:
                break
            
            expression = match.group(1)
            result = str(self._evaluate_expression_with_parentheses(expression))
            message = message[:match.start()] + result + message[match.end():]
        
        return message
    
    def _handle_exit(self, parts: List[str], line_number: int):
        """Handle EXIT command"""
        if len(parts) == 1:
            print("Exiting program... Goodbye!")
            sys.exit(0)
        elif len(parts) == 2:
            message = parts[1].strip('"')
            print(message)
            sys.exit(0)
        else:
            raise NoobieError("EXIT command requires at most one argument")
    
    def _handle_say(self, parts: List[str], line_number: int):
        """Handle SAY command"""
        if len(parts) < 2:
            raise NoobieError("SAY command requires a message")
        
        # Join all parts after 'say' to handle spaces in messages
        message = ' '.join(parts[1:]).strip('"')
        
        # Check for extra quotes
        if message.count('"') > 0:
            raise NoobieError("Extra characters after message in SAY command")
        
        message = self._extract_expression(message)
        print(message)
    
    def _handle_set(self, parts: List[str], line_number: int):
        """Handle SET command with improved parsing"""
        if len(parts) < 3:
            raise NoobieError("SET command requires at least 2 arguments")
        
        is_const = parts[1].lower() == 'const'
        offset = 2 if is_const else 1
        
        if len(parts) < offset + 2:
            raise NoobieError(f"SET {'CONST ' if is_const else ''}command requires type and variable name")
        
        var_type = parts[offset]
        var_name = parts[offset + 1]
        value_raw = parts[offset + 2] if len(parts) > offset + 2 else None
        
        if var_name == "heared":
            raise NoobieError("Cannot use 'heared' as variable name")
        
        if var_name in self.variables and self.variables[var_name].const:
            raise NoobieError(f"Cannot redeclare constant variable: '{var_name}'")
        
        value = initialize_variable(var_type, value_raw)
        self.variables[var_name] = Variable(var_type.upper(), value, is_const)
    
    def _handle_hear(self, parts: List[str], line_number: int):
        """Handle HEAR command"""
        if len(parts) < 3:
            raise NoobieError("HEAR command requires type and message")
        
        var_type = parts[1]
        message = ' '.join(parts[2:]).strip('"')
        
        user_input = input(f"{message}: ")
        value = initialize_variable(var_type, user_input)
        self.variables['heared'] = Variable(var_type.upper(), value)
    
    def _handle_change(self, parts: List[str], line_number: int):
        """Handle CHANGE command"""
        if len(parts) < 3:
            raise NoobieError("CHANGE command requires variable name and new value")
        
        var_name = parts[1]
        new_value_raw = ' '.join(parts[2:])
        
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        if self.variables[var_name].const:
            raise NoobieError(f"Cannot modify constant variable: '{var_name}'")
        
        var_type = self.variables[var_name].type
        
        # Handle expressions in parentheses
        if new_value_raw.startswith('(') and new_value_raw.endswith(')'):
            new_value = self._evaluate_expression_with_parentheses(new_value_raw[1:-1])
        else:
            new_value = initialize_variable(var_type, new_value_raw)
        
        self.variables[var_name].value = new_value
    
    def _handle_convert(self, parts: List[str], line_number: int):
        """Handle CONVERT command"""
        if len(parts) != 3:
            raise NoobieError("CONVERT command requires variable name and new type")
        
        var_name, new_type = parts[1], parts[2]
        
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        if self.variables[var_name].const:
            raise NoobieError(f"Cannot modify constant variable: '{var_name}'")
        
        old_var = self.variables[var_name]
        new_value = convert_value(old_var.value, old_var.type, new_type.upper())
        self.variables[var_name] = Variable(new_type.upper(), new_value, old_var.const)
    
    def _handle_random(self, parts: List[str], line_number: int):
        """Handle RANDOM command"""
        if len(parts) < 4:
            raise NoobieError("RANDOM command requires type, min, and max values")
        
        var_type, min_val, max_val = parts[1], int(parts[2]), int(parts[3])
        result = randomize(min_val, max_val, var_type)
        
        if len(parts) == 4:
            print(result)
        elif len(parts) == 5:
            var_name = parts[4]
            self.variables[var_name] = Variable(var_type.upper(), result)
        else:
            raise NoobieError("RANDOM command has too many arguments")
    
    def _handle_round(self, parts: List[str], line_number: int):
        """Handle ROUND command"""
        if len(parts) != 3:
            raise NoobieError("ROUND command requires variable name and precision")
        
        var_name, precision = parts[1], int(parts[2])
        
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        if self.variables[var_name].type != 'FLOAT':
            raise NoobieError("ROUND command requires a FLOAT variable")
        
        value = self.variables[var_name].value
        self.variables[var_name].value = round(value, precision)
    
    def _handle_del(self, parts: List[str], line_number: int):
        """Handle DEL command"""
        if len(parts) != 2:
            raise NoobieError("DEL command requires exactly one variable name")
        
        var_name = parts[1]
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        del self.variables[var_name]
    
    def _handle_reset(self, parts: List[str], line_number: int):
        """Handle RESET command"""
        if len(parts) != 2:
            raise NoobieError("RESET command requires exactly one variable name")
        
        var_name = parts[1]
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        var_type = self.variables[var_name].type
        self.variables[var_name].value = initialize_variable(var_type, None)
    
    def _handle_increment(self, parts: List[str], line_number: int):
        """Handle INCREMENT command"""
        if len(parts) != 2:
            raise NoobieError("INCREMENT command requires exactly one variable name")
        
        var_name = parts[1]
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        if self.variables[var_name].type not in ['INT', 'FLOAT']:
            raise NoobieError("INCREMENT requires INT or FLOAT variable")
        
        self.variables[var_name].value += 1
    
    def _handle_decrement(self, parts: List[str], line_number: int):
        """Handle DECREMENT command"""
        if len(parts) != 2:
            raise NoobieError("DECREMENT command requires exactly one variable name")
        
        var_name = parts[1]
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        if self.variables[var_name].type not in ['INT', 'FLOAT']:
            raise NoobieError("DECREMENT requires INT or FLOAT variable")
        
        self.variables[var_name].value -= 1
    
    def _handle_swap(self, parts: List[str], line_number: int):
        """Handle SWAP command"""
        if len(parts) != 3:
            raise NoobieError("SWAP command requires exactly two variable names")
        
        var1, var2 = parts[1], parts[2]
        
        if var1 not in self.variables or var2 not in self.variables:
            raise NoobieError("Both variables must be declared")
        
        if self.variables[var1].type != self.variables[var2].type:
            raise NoobieError("Both variables must have the same type")
        
        self.variables[var1].value, self.variables[var2].value = \
            self.variables[var2].value, self.variables[var1].value
    
    def _handle_string_operation(self, parts: List[str], line_number: int, operation: str):
        """Generic handler for string operations"""
        if len(parts) != 2:
            raise NoobieError(f"{operation} command requires exactly one variable name")
        
        var_name = parts[1]
        if var_name not in self.variables:
            raise NoobieError(f"Variable '{var_name}' not declared")
        
        var_type = self.variables[var_name].type
        valid_types = ['CHAR', 'STR'] if operation != 'REVERSE' else ['STR']
        
        if var_type not in valid_types:
            type_str = " or ".join(valid_types)
            raise NoobieError(f"{operation} command requires a {type_str} variable")
        
        value = self.variables[var_name].value
        
        if operation == 'UPPERCASE':
            result = value.upper()
        elif operation == 'LOWERCASE':
            result = value.lower()
        elif operation == 'REVERSE':
            result = value[::-1]
        
        self.variables[var_name].value = result
    
    def _handle_uppercase(self, parts: List[str], line_number: int):
        """Handle UPPERCASE command"""
        self._handle_string_operation(parts, line_number, 'UPPERCASE')
    
    def _handle_lowercase(self, parts: List[str], line_number: int):
        """Handle LOWERCASE command"""
        self._handle_string_operation(parts, line_number, 'LOWERCASE')
    
    def _handle_reverse(self, parts: List[str], line_number: int):
        """Handle REVERSE command"""
        self._handle_string_operation(parts, line_number, 'REVERSE')
    
    def _process_line(self, line: str, line_number: int):
        """Process a single line of code"""
        # Handle comment blocks
        if line.startswith('##'):
            if not self.in_comment_block:
                self.in_comment_block = True
                return
            else:
                self.in_comment_block = False
                return
        
        if self.in_comment_block:
            return
        
        # Remove single-line comments and strip whitespace
        line = line.split('#', 1)[0].strip()
        if not line:
            return
        
        # Replace variables with their values
        line = replace_variables(line, self.variables)
        
        # Parse command
        parts = line.split()
        if not parts:
            return
        
        command = parts[0].lower()
        
        # Handle commands using command handlers
        if command in self.command_handlers:
            try:
                self.command_handlers[command](parts, line_number)
            except NoobieError:
                raise
            except Exception as e:
                raise NoobieError(f"Error in {command.upper()} command: {e}")
        
        # Handle arithmetic expressions
        elif any(op in line for op in ['+', '-', '*', '/', '//', '%', '**', '==', '!=', '<', '>', 
                                       'AND', 'OR', 'NOT', 'XOR', 'and', 'or', 'not', 'xor']):
            try:
                result = self._evaluate_expression_with_parentheses(line)
                print(result)
            except NoobieError:
                raise
        else:
            raise NoobieError(f"Unknown command: {command}")
    
    def interpret(self, code: str):
        """Main interpretation method"""
        try:
            for line_number, line in enumerate(code.splitlines(), start=1):
                line = line.lower().strip()
                try:
                    self._process_line(line, line_number)
                except NoobieError as e:
                    e.line_number = line_number
                    raise
                    
        except NoobieError as e:
            handle_error(str(e), e.line_number)
        except Exception as e:
            handle_error(f"Unexpected error: {e}")

def main():
    """Main function"""
    if len(sys.argv) < 2:
        handle_error("Specify a .noob file")
    
    filename = sys.argv[1]
    
    try:
        code = read_code_from_file(filename)
        interpreter = NoobieInterpreter()
        interpreter.interpret(code)
    except NoobieError as e:
        handle_error(str(e))
    except Exception as e:
        handle_error(f"Error reading file: {e}")

if __name__ == '__main__':
    main()
