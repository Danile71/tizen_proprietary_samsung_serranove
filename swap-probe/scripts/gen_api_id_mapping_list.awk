# This script generates api_id_mapping text file from api list

function find_el_in_array(a, el)
{
	for (i in a)
		if (a[i] == el)
			return 1;

	return 0
}

BEGIN {
	api_id = 1
	tags[1] = "#filename "
	tags[2] = "#lib "
	tags[3] = "#feature "
	tags[4] = "#feature_always "
	cur[1] = ""
	cur[2] = ""
	cur[3] = ""
	cur[4] = ""

	print "/*"
	print " * this file genereted by <swap-probe> project with cmd <make headers>"
	print " */"
	print ""
	print "#ifdef __cplusplus"
	print "extern \"C\"{"
	print "#endif"
	print ""
	print "#define X_API_MAP_LIST \\"

	handled_ids_cnt = 1
	handled_ids[1] = ""
} {
	if ( $0 != ""  ) {
		if ( substr($0,1,1) == "#" ) {
			#-> #filename
			#-> #lib
			#-> #feature
			for (i = 1; i <= 4; i = i + 1) {
				tag = tags[i]
				if (substr($0,1,length(tag)) == tag) {
					split($0, splited, tag)
					#printf "%s=%s\n", tag, splited[2]
					cur[i] = splited[2]
					if (i == 1) {
						cur[2] = "\"???\""
						cur[3] = "0"
						cur[4] = "0"
					} else if ((i == 3 || i == 4) && cur[i] == "---") {
						cur[i] = "0"
					}
				}
			}
			#printf "%s\n", $0
		} else {
			api_name = $0

			split(api_name, splited, ";")
			api_name = splited[2]
			gsub(/^[ ]*/, "", api_name)
			if (find_el_in_array(handled_ids, api_name) == false) {
				printf "X(%d, \"%s\", %s, %s, %s, %s) \\\n", api_id, api_name, cur[1], cur[2], cur[3], cur[4]
				api_id++
				handled_ids[handled_ids_cnt] = api_name
				handled_ids_cnt++
			}
		}
	}
}

END {
	print ""
	print "#ifdef __cplusplus"
	print "}"
	print "#endif"
}
