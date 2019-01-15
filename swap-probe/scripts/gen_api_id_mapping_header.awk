# This script generates api_id_mapping header from api list

function find_el_in_array(a, el)
{
	for (i in a)
		if (a[i] == el)
			return 1;

	return 0
}

BEGIN {
	print "/*"
	print " * this file genereted by <swap-probe> project with cmd <make headers>"
	print " */"
	print ""
	print "#ifndef __API_ID_MAP__"
	print "#define __API_ID_MAP__"
	print ""
	print "#ifdef __cplusplus"
	print "extern \"C\"{"
	print "#endif"
	print ""
	api_id = 1
	macro_prefix = "API_ID_"
	created_defs_cnt = 1
	created_defs[1] = ""
} {
	orig = $0

	if ( orig == "" ) {
		print ""
	} else if ( substr(orig,1,1) == "#" ) {
		gsub("\*", "\\*", orig)
		printf "/* %s */\n", orig
	} else {
		tokens_len = split(orig, tokens, ";")

		if (tokens_len != 3) {
			printf "//#warning %-135s  //bad string \n", orig
		} else {

			def = tokens[2]
			split(def, splited, "###")
			def = splited[1]
			gsub(/^[ ]*/, "", def)
			gsub(/[,:()*&~\[\] ]/, "_", def)
			def = macro_prefix def
			if (find_el_in_array(created_defs, def) == false) {
				printf "#define %-135s %d // %s\n", def, api_id, tokens[1]
				api_id++
				created_defs[created_defs_cnt] = def
				created_defs_cnt++
			}
		}
	}
}
END {
	print ""
	print "#ifdef __cplusplus"
	print "}"
	print "#endif"
	print ""
	print "#endif /* __API_ID_MAP__ */"
}
