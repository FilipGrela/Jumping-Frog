import re
from tabulate import tabulate
from colorama import Fore, Style, init
import argparse

# Inicjalizacja kolorów (colorama)
init()

# Łatwo edytowalny próg długości funkcji
MAX_LENGTH = 2**10

def analyze_cpp_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()

    results = []
    function_name = None
    function_body = []
    inside_function = False
    brace_count = 0

    # Wyrażenie regularne do rozpoznania nagłówka funkcji
    function_header_pattern = re.compile(r'(\w[\w\s*&]+)\s+(\w+)\s*\([^)]*\)\s*{')

    for line in lines:
        stripped_line = line.strip()

        if not inside_function:
            # Szukaj początku funkcji
            match = function_header_pattern.match(stripped_line)
            if match:
                function_name = match.group(2)  # Nazwa funkcji
                inside_function = True
                brace_count = stripped_line.count('{') - stripped_line.count('}')
                function_body = []
        else:
            # Jesteśmy wewnątrz funkcji - liczymy klamry
            brace_count += stripped_line.count('{') - stripped_line.count('}')
            function_body.append(stripped_line)

            if brace_count == 0:
                # Funkcja zakończona
                cleaned_body = re.sub(r'\s+', '', ''.join(function_body))
                function_length = len(cleaned_body)
                results.append([function_name, function_length])
                inside_function = False
                function_name = None
                function_body = []

    # Formatowanie wyników
    formatted_results = []
    for name, length in results:
        color = Fore.RED if length > MAX_LENGTH else Fore.GREEN
        formatted_results.append([f"{color}{name}{Style.RESET_ALL}", f"{color}{length}{Style.RESET_ALL}"])

    # Wyświetlenie wyników w tabeli
    if formatted_results:
        print(tabulate(formatted_results, headers=["Nazwa funkcji", "Długość (bez białych znaków)"], tablefmt="grid"))
    else:
        print("Nie znaleziono żadnych funkcji w pliku.")

if __name__ == "__main__":
    # Parsowanie argumentów
    parser = argparse.ArgumentParser(description="Analizator długości funkcji w plikach C++.")
    parser.add_argument("file", help="Ścieżka do pliku C++ (.cpp) do analizy.")
    args = parser.parse_args()

    # Wywołanie funkcji analizy
    analyze_cpp_file(args.file)
