s/(#[^)][^)]*)//g
#s/(\$[^)][^)]*)//g
s/(''[^)][^)]*)//g
s/([^()][^()]* -R[ACRS]B-)//g
s/([^()][^()]* -L[ACRS]B-)//g
s/([^()][^()]* \/)//g
s/([^()][^()]* _)//g
s/(,[^)][^)]*)//g
s/(\.[^)][^)]*)//g
s/(:[^)][^)]*)//g
s/(``[^)][^)]*)//g
s/[ ][ ]*/ /g
s/) )/))/g
#--- replace all the other occurences of slash and underscore with symbols -SLASH- and -UNDERSCORE- ---#
s/_/-UNDERSCORE-/g
s/\//-SLASH-/g
