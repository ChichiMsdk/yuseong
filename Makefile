FILE := Makefile
OS_EXT =
ifeq ($(OS),Windows_NT)
	OS_EXT := .win32
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
		OS_EXT := .linux
    endif
    ifeq ($(UNAME_S),Darwin)
		OS_EXT := .mac
    endif
endif

include $(FILE)$(OS_EXT)

.PHONY: all re clean fclean

# all:
# 	make --no-print-directory -f $(FILE)$(OS_EXT)
#
# re:
# 	make --no-print-directory -f $(FILE)$(OS_EXT) re
#
# compile_commands.json:
# 	make --no-print-directory -f $(FILE)$(OS_EXT) compile_commands.json
#
# clean:
# 	make --no-print-directory -f $(FILE)$(OS_EXT) clean
#
# fclean:
# 	make --no-print-directory -f $(FILE)$(OS_EXT) fclean
#

