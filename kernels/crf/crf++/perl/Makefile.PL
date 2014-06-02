use ExtUtils::MakeMaker;
WriteMakefile(
    'NAME'		=> 'CRFPP',
    'CC'                => 'c++',
    'LD'                => 'c++',
    'INC'               => '',
    'LIBS'              => '-lpthread -lcrfpp',
#    'VERSION'	        => '0.46',
    'OBJECT'            => 'CRFPP_wrap.o' 
);
