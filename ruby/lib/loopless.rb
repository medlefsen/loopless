module Loopless

  class Error < RuntimeError
    attr_reader :code
    def initialize(code)
      @code = code
      super "Loopless failed with errorcode #{code}"
    end
  end

  def self.platform
    if RUBY_PLATFORM =~ /darwin|mac os x/
      :darwin
    elsif RUBY_PLATFORM =~ /linux/
      :linux
    else
      raise RuntimeError.new "Invalid loopless platform. Only Mac/Linux supported."
    end
  end

  def self.binary_path
    File.expand_path("../../#{platform}/loopless",__FILE__)
  end

  def self.version
    @version ||= run('version')
  end

  def self.create(file,size)
    run file, 'create', size.to_s
    nil
  end

  def self.create_vmdk(file,vmdk_path)
    run file, 'create-vmdk', vmdk_path
    nil
  end

  def self.create_gpt(file)
    run file, 'create-gpt'
    nil
  end

  def self.create_part(file, part_num: nil, start: nil, size: nil, label: nil)
    run file, 'create-part', *[part_num||0,start||0,size||0,label].compact.map(&:to_s)
    nil
  end

  def self.part_info(file)
    run(file, 'part-info').split("\n").map do |l|
      fields = l.split("\t")
      {
        number: fields[0].to_i,
        start: fields[1].to_i,
        size: fields[2].to_i,
        bootable: fields[3] == 'true',
        label: fields[4] != '' ? fields[4] : nil
      }
    end
  end

  def self.install_mbr(file)
    run file, 'install-mbr'
    nil
  end

  def self.set_bootable(file,part_num,value = true)
    run "#{file}:#{part_num}", "set-bootable", (value ? 'true' : 'false')
    nil
  end

  def self.syslinux(file,part=nil)
    run_part file,part,'syslinux'
    nil
  end

  def self.mkfs(file,*args)
    part = nil
    label = []
    if args.size == 1
      if args[0].is_a?(Array) || args[0].is_a?(Integer)
        part = args[0]
      else
        label = args
      end
    elsif args.size == 2
      part,label[0] = *args
    end

    run_part file,part,'mkfs', *label
    nil
  end

  def self.ls(file,part=nil)
    run_part(file,part,'ls').split("\n")
  end

  def self.rm(file,part=nil,path)
    run_part file,part,'rm',fix_path(path)
    nil
  end

  def self.read(file,part=nil,path)
    run_part file,part,'read',fix_path(path)
  end

  def self.write(file,*args)
    part = nil
    perm = '644'
    case args.size
    when 2 then path,input = args
    when 3
      has_perm = args[1].to_s =~ /^[0-8]{3}$/
      has_part = args[0].is_a?(Integer) || args[0].is_a?(Array)
      if has_perm && !has_part
        path,perm,input=args
      elsif !has_perm && has_part
        part,path,input=args
      else
        raise ArgumentError.new "Ambigious arguments"
      end
    when 4
      part,path,perm,input = args
    end
    run_part file,part,'write',fix_path(path),perm.to_s, in: input
    nil
  end

  def self.fix_path(path)
    if path[0] == '/'
      path
    else
      '/' + path
    end
  end

  def self.fmt_part(file,part)
    if part == nil
      file
    elsif part.respond_to? :to_i
      "#{file}:#{part.to_i}"
    elsif part.respond_to?(:to_a) && (part=part.to_a) && part.size == 2 && part.all?{|e| e.is_a? Integer}
      "#{file}:#{part[0]},#{part[1]}"
    else
      raise ArgumentError.new "Invalid loopless part: #{part.inspect}"
    end
  end

  def self.run_part(file,part,*command,**opts)
    run(fmt_part(file,part),*command,**opts)
  end
  
  def self.run(*command, **opts)
    out_read, out_write = IO.pipe
    input = if opts[:in].respond_to? :fileno
              opts[:in]
            elsif opts[:in]
              in_read,in_write = IO.pipe
              in_read
            else
              '/dev/null'
            end
    pid = spawn(binary_path,*command,in: input, out: out_write,err: '/dev/null')
    out_write.close
    if in_write
      in_read.close
      in_write.write opts[:in]
      in_write.close
    end
    output = out_read.read.strip
    out_read.close
    Process.wait pid
    raise Loopless::Error.new($?.exitstatus) unless $?.success?
    output
  end
end
