require 'mkmf'

crfpp_config = with_config('crfpp-config', 'crfpp-config')
use_crfpp_config = enable_config('crfpp-config')
have_library("crfpp")
have_library("pthread")
have_header('crfpp.h') && create_makefile('CRFPP')
