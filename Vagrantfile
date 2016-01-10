Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/trusty64"
  config.vm.synced_folder ".", "/vagrant", disabled: true
  config.vm.provider "virtualbox" do |v|
    v.memory = 1024
  end
  config.vm.provision "shell", inline: <<-SHELL
    REPOSITORY="mkok-libevent-ng"
    set -e
    sudo apt-get install -y git                                             && \
      git clone https://github.com/bassosimone/$REPOSITORY                  && \
      cd $REPOSITORY                                                        && \
      sudo apt-get install -y `cat apt-requirements.txt`                    && \
      ./autogen.sh                                                          && \
      ./script/mkpm install `cat mkpm-requirements.txt`                     && \
      ./script/mkpm shell ./configure                                       && \
      make V=0                                                              && \
      make check V=0
  SHELL
end
