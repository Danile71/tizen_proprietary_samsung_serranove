prefix ?= /usr
datadir ?= $(prefix)/share
sysconfdir ?= /etc

ELM_PROFILE ?= mobile
PROFILE_DIR = $(DESTDIR)/$(sysconfdir)/profile.d/
THEME_DIR = $(DESTDIR)/$(datadir)/themes/
DT_FORMAT_DIR = $(DESTDIR)/$(datadir)/datetimeformats/
CONFIG_DIR = $(DESTDIR)/$(datadir)/elementary/config/$(ELM_PROFILE)

INSTALL = install -c
EET_EET = eet

all:
	cd $(TARGET)/$(SIZE)/profile.d && \
		echo "export ELM_PROFILE="\"$(ELM_PROFILE)\" >> elm.sh ; \
		echo "export ELM_ATSPI_MODE=1" >> elm.sh ; \
		if [ $(EFL_ABORT_ENABLE) = "on" ]; then \
		    echo "export EINA_LOG_ABORT=1" >> eina.sh ; \
		    echo "export EINA_LOG_ABORT_LEVEL=1" >> eina.sh ; \
		fi ; \
	cd - ;\
	cd $(TARGET)/$(SIZE)/config && \
		$(EET_EET) -e base.cfg config base.src 1 ; \
		$(EET_EET) -e color.cfg config color.src 1 ; \
		$(EET_EET) -e font.cfg config font.src 1 ; \
	cd - ; \

clean:
	cd $(TARGET)/$(SIZE)/config && \
	rm -rf *.cfg
	cd - ; \

install:
	cd $(TARGET) && \
	mkdir -p $(THEME_DIR) $(DT_FORMAT_DIR) && \
	$(INSTALL) common/themes/*.xml $(THEME_DIR) && \
	$(INSTALL) common/datetimeformats/DatetimeFormats.xml $(DT_FORMAT_DIR) && \
	cd - ; \
	cd $(TARGET)/$(SIZE) && \
		mkdir -p $(PROFILE_DIR) $(CONFIG_DIR) && \
		$(INSTALL) profile.d/*.sh $(PROFILE_DIR) && \
		$(INSTALL) config/*.cfg $(CONFIG_DIR) && \
		$(INSTALL) config/profile.desktop $(CONFIG_DIR) && \
		$(INSTALL) config/icon.png $(CONFIG_DIR) ; \
	cd - ; \
