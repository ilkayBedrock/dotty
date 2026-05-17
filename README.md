# CFMAN

# DEPS
git, github-cli

# CONVENTIONS

## HEADER FILES
I name a header with *.hpp if it has/planned to have implementaton in a *.cpp,
otherwise it is named as *.h, not because it is C header but it does not need
implementation to it(such as templates, inline functions, etc.)

## SYMBOLS/TOKENS
- Variables/Constants/Members: descriptive, snake_case
- Functions/Methods: camelCase
- Classes/Typedefs: PascalCase
- Macros(optionally constexprs): UPPER_SNAKE_CASE


