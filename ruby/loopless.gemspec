Gem::Specification.new do |s|
  s.name        = 'loopless'
  s.version     = ENV['VERSION'] or raise 'no version'
  s.licenses    = ['GPL-2.0']
  s.summary     = "Simple bootable disk image utility."
  s.description = "Fully create bootable disk images without loopback mounts or root."
  s.authors     = ["Matt Edlefsen"]
  s.email       = 'matt.edlefsen@gmail.com'
  s.files       = ["bin/loopless","darwin/loopless","linux/loopless","lib/loopless.rb","README.md","COPYING"]
  s.executables = ['loopless']
  s.homepage    = 'https://github.com/medlefsen/loopless'
end
