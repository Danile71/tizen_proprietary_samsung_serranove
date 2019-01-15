NAME=setup
VERSION := $(shell awk '/Version:/ { print $$2 }' $(NAME).spec)
TAG=$(NAME)-$(VERSION)


check:
	@echo Sanity checking selected files....
	bash -n bashrc
	bash -n profile
	./uidgidlint ./uidgid
	./serviceslint ./services

tag-archive: check
	@git tag -a -m "Tag as $(TAG)" -f $(TAG)

create-archive:
	@git-archive --format=tar --prefix=$(NAME)-$(VERSION)/ HEAD | bzip2 > $(NAME)-$(VERSION).tar.bz2
	@echo "The archive is at $(NAME)-$(VERSION).tar.bz2"

archive: tag-archive create-archive

clean:
	rm -f *.bz2
