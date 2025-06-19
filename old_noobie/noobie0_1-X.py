#TARANTOLA LUCA
#VERSIONE 0.1.X NOOBIE INTERPRETER 
#ULTIMA MODIFICA:
#COMANDI
#SET - set
#SAY - say
#HEAR - hear
#CONVERT - convert
#CHANGE - change
#RANDOM - random
#ROUND - round
#DEL - del
#RESET - reset
#INCREMENT - increment
#DECREMENT - decrement
#SWAP - swap
#UPPERCASE - uppercase
#LOWERCASE - lowercase
#REVERSE - reverse



import sys
from funzioni import *

def interpret(code):
    """Interpreta ed esegue un insieme di istruzioni in formato stringa."""
    stack = {} # Dizionario per memorizzare le variabili dichiarate.
    in_comment_block = False # Stato per tracciare i blocchi di commento

    def evaluate_expression_with_parentheses(expression):
        """Questa funzione calcola il risultato dell'espressione aritmetica gestendo le parentesi."""
        try:
            return evaluate_expression(expression, stack)
        except ValueError as e:
            handle_error(f"Error while evaluating expression: {e}")

    def extract_expression(message):
        """Estrae ed esegue le espressioni tra parentesi graffe {}"""
        pattern = r'\{([^{}]+)\}'  # Cerca contenuto tra parentesi graffe
        while re.search(pattern, message):
            match = re.search(pattern, message)
            if not match:
                break
            expression = match.group(1)
            result = str(evaluate_expression_with_parentheses(expression))
            message = message[:match.start()] + result + message[match.end():]
        return message

    try: 
        
            # Itera su ogni riga di codice sorgente
            for line_number, line in enumerate(code.splitlines(), start=1):
                line = line.lower()
                line = line.strip()

                # Gestione dei blocchi di commento che iniziano con '##'
                if line.startswith('##') and not in_comment_block:
                    in_comment_block = True
                    continue
                elif line.endswith('##') and in_comment_block:
                    in_comment_block = False
                    continue
                elif in_comment_block:
                    continue    

                # Rimuovi i commenti da una riga di codice e ignora le righe vuote
                line = line.split('#', 1)[0].strip()
                if not line:
                    continue

                # Sostituisce le variabili nella riga con i loro valori
                line = replace_variables(line, stack)

                # Comando EXIT: termina l'esecuzione del programma
                if line.startswith('exit') or line.startswith('EXIT'):
                    parts = line.split(maxsplit=1)

                    if len(parts) == 1:
                        print("Exiting program... Goodbye!")
                        sys.exit(0)
                    elif len(parts) == 2:
                        message = parts[1].strip('"')
                        print(message)
                        sys.exit(0)
                    else:
                        handle_error("EXIT commando require at most one argument", line_number)

                # Comando SAY: stampa un messaggio a schermo
                elif line.startswith('say') or line.startswith('SAY'):
                    try:
                        message = line[4:].strip('"')
                        
                        # Controlla che non ci siano caratteri extra dopo le virgolette
                        if '"' in message[1:]:
                            handle_error("Error: Extra characters after the message in SAY command.", line_number)

                        # Valuta eventuali espressioni contenute nel messaggio
                        message = extract_expression(message)
                        print(message)
                    except Exception as e:
                        handle_error(f"Error in SAY command: {e}", line_number)

                # Comando SET: dichiara una variabile
                elif line.startswith('set') or line.startswith('SET'):
                    try:
                        parts = line.split(maxsplit=4)

                        if not parts or len(parts) < 3:
                            handle_error("SET command requires at least 2 arguments.", line_number)

                        is_const = False

                        #SET CONST <type> <var_name> <value>
                        if parts[1].lower() == 'const':
                            is_const = True
                             
                            if len(parts) == 4:
                                _, _, type, var_name = parts
                                value = initialize_variable(type, None)
                            elif len(parts) == 5:
                                _, _, type, var_name, value_raw = parts
                                value = initialize_variable(type, value_raw)
                            else:
                                handle_error("SET CONST command requires 2 or 3 arguments.", line_number)
                        
                        #SET <type> <var_name> <value>
                        else:
                            if len(parts) == 3:
                                _, type, var_name = parts
                                value = initialize_variable(type, None)
                            elif len(parts) == 4:
                                _, type, var_name, value_raw = parts
                                value = initialize_variable(type, value_raw)
                            else:
                                handle_error("SET command requires 2 or 3 arguments.", line_number)

                        if var_name == "heared":
                            handle_error("Cannot use 'heared' as a variable name.", line_number)
                        
                        stack[var_name] = {'type': type.upper(), 'value': value, 'const': is_const}

                    except ValueError as e:
                        handle_error(f"SET error: {e}", line_number)

                # Comando HEAR: legge un input utente e lo assegna a una variabile
                elif line.startswith('hear') or line.startswith('HEAR'):
                    try:
                        _, type, message = line.split(maxsplit=2)
                        message = message.strip('"')
                        user_input = input(f"{message}: ")
                        
                        # Inizializza il valore della variabile basandosi sul tipo specificato
                        stack['heared'] = {'type': type, 'value': initialize_variable(type, user_input)}
                    
                    except ValueError as e:
                        handle_error(f"HEAR error: {e}", line_number)
                    
                # Comando CHANGE: aggiorna il valore di una variabile esistente
                elif line.startswith('change') or line.startswith('CHANGE'):
                    try: 
                        _, var_name, new_value_raw = line.split(maxsplit=2)

                         # Controlla che la variabile non sia costante
                        if stack[var_name].get('const', False):
                            raise ValueError(f"Cannot modify a constant variable: '{var_name}'.")

                        # Controlla che la variabile esista
                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        type = stack[var_name]['type']

                        # Valuta un'espressione o cambia direttamente il valore
                        if new_value_raw.startswith('(') and new_value_raw.endswith(')'):
                            new_value = evaluate_expression(new_value_raw[1:-1], stack)
                        else:
                            new_value = initialize_variable(type, new_value_raw)

                        # Aggiorna il valore della variabile
                        stack[var_name]['value'] = new_value

                    except ValueError as e:
                        handle_error(f"CHANGE error: {e}", line_number)

                # Comando CONVERT: cambia il tipo di una variabile esistente
                elif line.startswith('convert') or line.startswith('CONVERT'):
                    try:
                        _, var_name, new_type = line.split(maxsplit=2)
                        
                        # Controlla che la variabile esista
                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        # Controlla che la variabile non sia costante
                        if stack[var_name].get('const', False):
                            handle_error(f"Cannot modify a constant variable: '{var_name}'.", line_number)

                        old_type = stack[var_name]['type']
                        current_value = stack[var_name]['value']

                        # Converte il valore al nuovo tipo e aggiorna la variabile
                        new_value = convert_value(current_value, old_type, new_type.upper())
                        stack[var_name] = {'type': new_type.upper(), 'value': new_value}

                    except ValueError as e:
                        handle_error(f"CONVERT error: {e}", line_number)
                        
                # Comando RANDOM: genera un valore casuale tra min e max, inclusi
                elif line.startswith('random') or line.startswith('RANDOM'):
                    try:
                        parts = line.split()
                        if len(parts) < 4:
                            handle_error("RANDOM command requires at least 2 arguments.", line_number)
                        elif len(parts) == 4:
                            _, type, min_val, max_val = parts
                            result = randomize(int(min_val), int(max_val), type)
                            print(result)
                        elif len(parts) == 5:
                            _, type, min_val, max_val, var_name = parts
                            result = randomize(int(min_val), int(max_val), type)
                            stack[var_name] = {'type': type.upper(), 'value': result}

                    except ValueError as e:
                        handle_error(f"RANDOM error: {e}", line_number)

                # Comando ROUND: arrotonda un numero float
                elif line.startswith('round') or line.startswith('ROUND'):
                    try:
                        parts = line.split()
                        if len(parts) != 3:
                            handle_error("ROUND command requires exactly 3 arguments.", line_number)

                        _, var_name, precision = parts

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] != 'FLOAT':
                            handle_error(f"ROUND command requires a FLOAT variable.", line_number)

                        value = stack[var_name]['value']
                        result = round(value, int(precision))
                        stack[var_name]['value'] = result
                    except ValueError as e:
                        handle_error(f"ROUND error: {e}", line_number)

                # Comando DEL: elimina una variabile
                elif line.startswith('del') or line.startswith('DEL'):
                    try:
                        var_name = line.split()[1]

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        del stack[var_name]
                    except ValueError as e:
                        handle_error(f"DEL error: {e}", line_number)

                # Comando RESET: riporta una variabile a 0
                elif line.startswith('reset') or line.startswith('RESET'):
                    try:
                        var_name = line.split()[1]

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        stack[var_name]['value'] = initialize_variable(stack[var_name]['type'], None)
                        
                    except ValueError as e:
                        handle_error(f"RESET error: {e}", line_number)


                # Comando INCREMENT: incrementa il valore di una variabile
                elif line.startswith('increment') or line.startswith('INCREMENT'):
                    try:
                        var_name = line.split()[1]

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] not in ['INT', 'FLOAT']:
                            handle_error(f"INCREMENT command requires an INT or FLOAT variable.", line_number)

                        stack[var_name]['value'] += 1

                    except ValueError as e:
                        handle_error(f"INCREMENT error: {e}", line_number)

                # Comando DECREMENT: decrementa il valore di una variabile
                elif line.startswith('decrement') or line.startswith('DECREMENT'):
                    try:
                        var_name = line.split()[1]

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] not in ['INT', 'FLOAT']:
                            handle_error(f"DECREMENT command requires an INT or FLOAT variable.", line_number)

                        stack[var_name]['value'] -= 1

                    except ValueError as e:
                        handle_error(f"DECREMENT error: {e}", line_number)

                # Comando SWAP: scambia i valori di due variabili
                elif line.startswith('swap') or line.startswith('SWAP'):
                    try:
                        _, var1, var2 = line.split()

                        if var1 not in stack or var2 not in stack:
                            handle_error("Both variables must be declared.", line_number)

                        type1 = stack[var1]['type']
                        type2 = stack[var2]['type']

                        if type1 != type2:
                            handle_error("Both variables must have the same type.", line_number)

                        stack[var1]['value'], stack[var2]['value'] = stack[var2]['value'], stack[var1]['value']

                    except ValueError as e:
                        handle_error(f"SWAP error: {e}", line_number)

                # Comando UPPERCASE: converte una stringa o un carattere in maiuscolo
                elif line.startswith('uppercase') or line.startswith('UPPERCASE'):
                    try:
                        _, var_name = line.split()

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] not in ['CHAR', 'STR']:
                            handle_error("UPPERCASE command requires a CHAR or STR variable.", line_number)

                        value = stack[var_name]['value']
                        result = value.upper()

                        stack[var_name]['value'] = result

                    except ValueError as e:
                        handle_error(f"UPPERCASE error: {e}", line_number)

                # Comando LOWERCASE: converte una stringa o un carattere in minuscolo
                elif line.startswith('lowercase') or line.startswith('LOWERCASE'):
                    try:
                        _, var_name = line.split()

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] not in ['CHAR', 'STR']:
                            handle_error("LOWERCASE command requires a CHAR or STR variable.", line_number)

                        value = stack[var_name]['value']
                        result = value.lower()

                        stack[var_name]['value'] = result

                    except ValueError as e:
                        handle_error(f"LOWERCASE error: {e}", line_number)

                # Comando REVERSE: inverte una stringa
                elif line.startswith('reverse') or line.startswith('REVERSE'):
                    try:
                        _, var_name = line.split()

                        if var_name not in stack:
                            handle_error(f"Variable '{var_name}' not declared.", line_number)

                        if stack[var_name]['type'] != 'STR':
                            handle_error("REVERSE command requires a STR variable.", line_number)

                        value = stack[var_name]['value']
                        result = value[::-1]

                        stack[var_name]['value'] = result

                    except ValueError as e:
                        handle_error(f"REVERSE error: {e}", line_number)

                # Valutazione di espressioni aritmetiche contenenti operatori matematici
                elif any(op in line for op in ['+', '-', '*', '/', '//', '%', '**', '==', '!=', '<', '>', 'AND', 'OR', 'NOT', 'XOR', 'and', 'or', 'not', 'xor']):
                    try:
                        print(evaluate_expression(line, stack))
                    except ValueError as e:
                        handle_error(f"Error while evaluating expression: {e}", line_number)
    except Exception as e:
        handle_error(f"General error: {e}")

if __name__ == '__main__': # Verifica se il modulo è eseguito come script principale
    if len(sys.argv) < 2: # Controlla che sia stato fornito almeno un argomento da riga di comando
        handle_error("Error: specify a .noob file ") # Messaggio di errore se manca il file richiesto
    
    file_name = sys.argv[1] # Nome del file da eseguire
    try:
        code= read_code_from_file(file_name) # Legge il codice sorgente dal file
        interpret(code) # Esegue il codice sorgente
    except Exception as e:
        handle_error(f"Error while reading file: {e}") # Messaggio di errore se si verifica un errore durante la lettura del file


