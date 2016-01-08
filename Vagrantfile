Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.synced_folder ".", "/vagrant", disabled: true
  config.vm.provider "virtualbox" do |v|
    v.memory = 1024
  end
  config.vm.provision "shell", inline: <<-SHELL
    APT_PACKAGES="autoconf g++ git libtool"
    MKPM_PACKAGES="libressl libevent mkok-base"
    REPOSITORY="mkok-libevent-ng"
    set -e
    sudo apt-get install -y $APT_PACKAGES                                   && \
      git clone https://github.com/bassosimone/$REPOSITORY                  && \
      cd $REPOSITORY                                                        && \
      ./autogen.sh                                                          && \
      ./script/mkpm install $MKPM_PACKAGES                                  && \
      ./script/mkpm shell ./configure                                       && \
      make V=0                                                              && \
      make check V=0
  SHELL
end
