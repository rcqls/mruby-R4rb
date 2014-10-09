module R4mrb

  def R4mrb.init
    @@initR=R4mrb.initR unless R4mrb.alive?
  end

  def R4mrb.eval(s,aff=0)
    s="{\n"+s+"}\n"
    evalLine s,aff
  end

  def R4mrb.parse(s,aff=0)
    s="{\n"+s+"}\n"
    parseLine s,aff
  end

  def R4mrb.try_eval(code)
    try_code=".result_try_code<-try({\n"+code+"\n},silent=TRUE)\n.result_try_code"
    R4mrb << try_code
    puts ".result_try_code".to_R if "inherits(.result_try_code,'try-error')".to_R
  end


  def R4mrb.<<(s)
    R4mrb.eval(s)
  end

  class RVector

    attr_accessor :name, :type, :arg

    def <<(name)
      if name.is_a? Symbol
        @name=name.to_s
        @type="var"
      else
        @name=name
        @type="expr"
      end
      return self
    end

    def arg=(arg)
      @arg=arg
    end

    #this method is the same as the previous one but return self! Let us notice that even by adding return self in the previous one
    # I could not manage to execute (rvect.arg="[2]").value_with_arg but fortunately rvect.set_arg("[2]").value_with_arg is working!
    def set_arg(arg)
      @arg=arg
      return self
    end
   
    def >(arr)
      res=self.get
#puts "res";p @name;p res
      if res
#puts "arr.class:";p arr.class
#puts "res.class";p res.class
        res=[res] unless res.is_a? Array
        arr.replace(res)
      else
        arr.clear
      end
      return self
    end

  end

  class Dyn

    @@rb2R={}
  
    def self.[](*arr)
      rb2R=Dyn.new(arr)
    end

    def self.rb2R(arr)
      @@rb2R[arr]
    end

    attr_reader :ary

    def initialize(ary=[])
      R4mrb.init #just in case!
      @ary=ary
      @rb2R = RVector.new("")
      @@rb2R[@ary]=@rb2R
    end

    def replace(ary)
      @ary.replace ary
      self
    end

    def method_missing(m,*args,&block)
      ##p "delegate "+m.inspect
      @ary.send m,*args,&block
    end

    # def length
    #   @ary.length
    # end

    # def <<(elt)
    #   @ary << elt
    #   self
    # end

    # def +(ary)
    #   @ary + ary
    # end

    def >(outR) #outR represents here an R object
      @rb2R << outR
      @rb2R < @ary
      return self
    end

    def <(outR) #outR represents here an R expression to execute and put inside the Array
      @rb2R << outR
      @rb2R > @ary
      return @ary
    end

  end

  @@out=Dyn[]
  
  def R4mrb.<(rcode)
    @@out.replace [] ##@@out=[] #important! it could otherwise remove
    @@out < rcode.to_s ##@@out.inR4mrb rcode.to_s
    return (@@out.ary.length<=1 ? @@out.ary[0] : @@out.ary) 
  end

  class << self
    alias output <
  end

end

class Array

    def rb2R!
      R4mrb::Dyn.new self
    end

    def >(outR) #outR represents here an R object
      rb2R = R4mrb::Dyn.rb2R(self)
      rb2R << outR
      rb2R < self
      return self
    end

    def <(outR) #outR represents here an R expression to execute and put inside the Array
      rb2R = R4mrb::Dyn.rb2R(self)
      rb2R << outR
      rb2R > self
      return self
    end

  end

class String

  def R4mrb
    R4mrb < self
  end

  alias to_R R4mrb
  alias evalR R4mrb
  alias Reval R4mrb
  alias R R4mrb

end

module RObj

  def RObj.<(rcode)
    return (R4mrb < rcode) 
  end

  def RObj.class(rname)
    RObj < "class(#{rname})"
  end

  def RObj.mode(rcode)
    RObj < "mode(#{rcode})"
  end

  # temporary R object saved in ruby object!
  def RObj.make(rcode,first=true)
    mode=RObj.mode(rcode)
    RObj < ".tmpRObj<-(#{rcode})" if first
    code=(first ? ".tmpRObj" : rcode.dup) 
    if ['numeric','logical','complex','character'].include?(mode)
      #if RObj.class(code)=="matrix"
      return (RObj< rcode)
    elsif mode=="list"
      robj={}
      tmpNames=RObj < "names(#{rcode})"
      tmpNames.each_with_index do |nam,i|
         key=(nam.empty? ? i : nam)
         rkey=(nam.empty? ? i : "\""+nam+"\"")
         robj[key]=RObj.make("#{rcode}[[#{rkey}]]",false)
        end
      return robj
    else
      return nil
    end
  end

  def RObj.<<(rcode) 
    RObj.make(rcode)
  end

  def RObj.exist?(rname)
    RObj < "exists(\"#{rname}\")"
  end

end