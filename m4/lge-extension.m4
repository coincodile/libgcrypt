##################################################################################
# Initialize the configuration environment fo extensions
##################################################################################
AC_DEFUN([AC_INIT_LGE],[
	lge_ciphers=""
	lge_pubkey_ciphers=""
	lge_digests=""
	lge_kdfs=""
	lge_random_modules=""
	# List of LO file to add into the Libgcrypt
	lge_ciphers_libs=""

	# List of all cipher id where loaded from extensions
	LGE_HEADER_CONFIG_CIPHERID_FILE="lge_gcrypt_cipherid.h"
	echo '/* Auto Generated Cipher ID */' > $LGE_HEADER_CONFIG_CIPHERID_FILE
	lge_header_cipherid=320
	# List of all preconditions to add in gcrypt header file
	LGE_HEADER_CONFIG_CONDITION_FILE="lge_gcrypt_pre.h"
	echo '/* Auto Generated Code */' > $LGE_HEADER_CONFIG_CONDITION_FILE
	# List of all ciphers to add into array
	LGE_HEADER_CONFIG_CIPHERLIST_FILE="lge_gcrypt_cipherlist.h"
	echo '/* List of extensions */' > $LGE_HEADER_CONFIG_CIPHERLIST_FILE
	# List of all ciphers defenistions
	LGE_HEADER_CONFIG_CIPHERINFODEF_FILE="lge_gcrypt_cipherinfodef.h"
	echo '/* Definition of extensions */' > $LGE_HEADER_CONFIG_CIPHERINFODEF_FILE
	# List of all enabled ciphers source files. It will be generated
	# automatically
	LGE_CIPHER_SOURCES=""


	# List of LO file to add into the Libgcrypt (PK)
	lge_pubkey_ciphers_libs=""
	LGE_HEADER_CONFIG_PUBKEY_CIPHERID_FILE=
	LGE_HEADER_CONFIG_PUBKEY_CIPHERID_FILE="lge_gcrypt_pubkey_cipherid.h"
	echo '/* Auto Generated Public Key Cipher ID */' > $LGE_HEADER_CONFIG_PUBKEY_CIPHERID_FILE
	lge_header_pubkey_cipherid=320
	LGE_HEADER_CONFIG_PUBKEY_CIPHERINFODEF_FILE="lge_gcrypt_pubkey_cipherinfodef.h"
	echo '/* Definition of public key extensions */' > $LGE_HEADER_CONFIG_PUBKEY_CIPHERINFODEF_FILE
	LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE="lge_gcrypt_pubkey_cipherlist.h"
	echo '/* List of extensions */' > $LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE
])
##################################################################################
# Adds new extension to the LGE
##################################################################################
AC_DEFUN([AC_LGE_EXT_CIPHER],[
	AC_REQUIRE([AC_INIT_LGE])
	name=m4_ifval($1, $1, "xempty")
	if test "$name" = "xempty"; then
		AC_MSG_ERROR([The cipher name is requried by LGE to add a new cipher extensions.])
	fi
	AC_MSG_CHECKING(Loading LGE extension for cipher $name)
	# Adding to list of availible cipher
	lge_ciphers="$lge_ciphers $name"
	AC_MSG_RESULT(ok)
])
AC_DEFUN([AC_LGE_EXT_PUBKEY_CIPHER],[
	AC_REQUIRE([AC_INIT_LGE])
	name=m4_ifval($1, $1, "xempty")
	if test "$name" = "xempty"; then
		AC_MSG_ERROR([The public key name is requried by LGE to add a new one.])
	fi
	AC_MSG_CHECKING(Loading LGE extension for public key $name)
	# Adding to list of availible cipher
	lge_pubkey_ciphers="$lge_pubkey_ciphers $name"
	AC_MSG_RESULT(ok)
])
##################################################################################
# Enables all extensions
##################################################################################
AC_DEFUN([AC_LGE_EXT_CIPHERS_ENALBE],[
	AC_REQUIRE([AC_INIT_LGE])
	enabledCiphers="$1"
	for cipher in $lge_ciphers; do
		AC_MSG_CHECKING(Enabling LGE extension for cipher $cipher)
		LIST_MEMBER($cipher, $enabledCiphers)
		if test "$found" = "1"; then
			# Enable the cipher and checks
			echo $(printf "// Enable a %s from extension" "${cipher}") >> $LGE_HEADER_CONFIG_CONDITION_FILE
			echo $(printf "\n#define USE_%s %s" "${cipher^^}" "1") >> $LGE_HEADER_CONFIG_CONDITION_FILE
			# Add cipher ID
			lge_header_cipherid=$((lge_header_cipherid+1))
			echo $(printf "\n\tGCRY_CIPHER_%s   = %d," "${cipher^^}" "$lge_header_cipherid") >> $LGE_HEADER_CONFIG_CIPHERID_FILE
			# Add cipher implementaion list
			echo "#if USE_${cipher^^}" >> $LGE_HEADER_CONFIG_CIPHERLIST_FILE
			echo "	&_gcry_cipher_spec_${cipher}," >> $LGE_HEADER_CONFIG_CIPHERLIST_FILE
			echo "#endif" >> $LGE_HEADER_CONFIG_CIPHERLIST_FILE
			# Add cipher implementaion in definision list
			echo "extern gcry_cipher_spec_t _gcry_cipher_spec_${cipher};" >> $LGE_HEADER_CONFIG_CIPHERINFODEF_FILE
			# Add cipher lib to link list
			lge_ciphers_libs="$lge_ciphers_libs ${cipher}.lo"
			# Load hardware specific libs
			#case "${host}" in
			#	x86_64-*-*)
			#     GCRYPT_CIPHERS="$GCRYPT_CIPHERS arcfour-amd64.lo"
			#	;;
			#esac
			AC_MSG_RESULT(enabled)
		else
			AC_MSG_RESULT(disabled)
		fi
	done
	AC_SUBST(LGE_CIPHER_SOURCES)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_CONDITION_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_CIPHERID_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_CIPHERLIST_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_CIPHERINFODEF_FILE)
])

AC_DEFUN([AC_LGE_EXT_PUBKEY_CIPHERS_ENALBE],[
	AC_REQUIRE([AC_INIT_LGE])
	enabledCiphers="$1"
	for cipher in $lge_pubkey_ciphers; do
		AC_MSG_CHECKING(Enabling LGE extension for public key cipher $cipher)
		LIST_MEMBER($cipher, $enabledCiphers)
		if test "$found" = "1"; then
			# Enable the cipher and checks
			echo $(printf "// Enable public key %s from extension" "${cipher}") >> $LGE_HEADER_CONFIG_CONDITION_FILE
			echo $(printf "\n#define USE_%s %s" "${cipher^^}" "1") >> $LGE_HEADER_CONFIG_CONDITION_FILE
			# Add cipher ID
			lge_header_pubkey_cipherid=$((lge_header_pubkey_cipherid+1))
			echo $(printf "\n\tGCRY_PK_%s   = %d," "${cipher^^}" "$lge_header_pubkey_cipherid") >> $LGE_HEADER_CONFIG_PUBKEY_CIPHERID_FILE
			# Add cipher implementaion list
			echo "#if USE_${cipher^^}" >> $LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE
			echo "	&_gcry_pubkey_spec_${cipher}," >> $LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE
			echo "#endif" >> $LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE
			# Add cipher implementaion in definision list
			echo "extern gcry_pk_spec_t _gcry_pubkey_spec_${cipher};" >> $LGE_HEADER_CONFIG_PUBKEY_CIPHERINFODEF_FILE
			# Add cipher lib to link list
			lge_pubkey_ciphers_libs="$lge_pubkey_ciphers_libs ${cipher}.lo"
			AC_MSG_RESULT(enabled)
		else
			AC_MSG_RESULT(disabled)
		fi
	done
	AC_SUBST_FILE(LGE_HEADER_CONFIG_CONDITION_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_PUBKEY_CIPHERID_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_PUBKEY_CIPHERLIST_FILE)
	AC_SUBST_FILE(LGE_HEADER_CONFIG_PUBKEY_CIPHERINFODEF_FILE)
])
