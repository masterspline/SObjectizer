require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.enveloped_msg.message_limits.transform_signal'

	cpp_source 'main.cpp'
}

