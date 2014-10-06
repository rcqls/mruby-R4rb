MRuby::Gem::Specification.new('mruby-R4rb') do |spec|
  spec.license = 'MIT'
  spec.author  = 'R Cqls'

  # Add GEM dependency mruby-parser.
  # The version must be between 1.0.0 and 1.5.2 .
  #spec.add_dependency('mruby-parser', '>= 1.0.0', '<= 1.5.2')

  # Use any version of mruby-uv from github.
  #spec.add_dependency('mruby-uv', '>= 0.0.0', :github => 'mattn/mruby-uv')

  # Use latest mruby-onig-regexp from github. (version requirements can be ignored)
  #spec.add_dependency('mruby-onig-regexp', :github => 'mattn/mruby-onig-regexp')

  prefix=`R RHOME`.strip
  p prefix
  spec.cc.flags << "-I#{prefix}/include"
  spec.linker.flags << "-L#{prefix}/lib -lR"
  # spec.cxx.flags << "-std=c++0x -fpermissive #{`fltk3-config --cflags`.delete("\n\r")}"
  # if ENV['OS'] == 'Windows_NT'
  #   fltk3_libs = "#{`fltk3-config --use-images --ldflags`.delete("\n\r").gsub(/ -mwindows /, ' ')} -lgdi32 -lstdc++".split(" ")
  # else
  #   fltk3_libs = "#{`fltk3-config --use-images --ldflags`.delete("\n\r")} -lstdc++".split(" ")
  # end
  # flags = fltk3_libs.reject {|e| e =~ /^-l/ }
  # libraries = fltk3_libs.select {|e| e =~ /-l/ }.map {|e| e[2..-1] }
  # spec.linker.flags << flags
  # spec.linker.libraries << libraries

end