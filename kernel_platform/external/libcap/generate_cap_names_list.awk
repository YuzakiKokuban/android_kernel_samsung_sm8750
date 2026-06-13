/#define[ \t](CAP[_A-Z]+)[ \t]+([0-9]+)[ \t]*$/ { printf "{\"%s\", %s},\n", tolower($2), $3; }
