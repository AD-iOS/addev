#!/bin/sh -


_command="${0##*/}"
export LANG=en_US.UTF-8

if ! id -u | grep -q '^0$'; then
	sudo "${0}" "$@"
	exit 0
fi
#echo -n '[*] Now check this script has been started multiple times at the same time...'
if mkdir "/tmp/${_command}" 2>>/dev/null; then
	#echo 'OK!' | lecho
	sleep 0
else
	if [ ! "${1}" = '--lite' ]; then
		echo "[*] Cannot start multiple \"${_command}\" at the same time, which may cause accidental damage"
	else
		echo "[*] Cannot start multiple \"${_command}\" at the same time"
	fi
	exit 0
fi
unset _check

_dpkg_path="$(apt-config dump | grep 'State::status')"
_dpkg_path="${_dpkg_path#*\"}"
_dpkg_path="${_dpkg_path%\/status*}"

_check_dpkg_lock(){
	_wait='1'
	while [ '1' -le '2' ]; do
		_lsof_dpkg="$(lsof "${_dpkg_path}/lock")"
		if [ -n "${_lsof_dpkg}" ]; then
			if [ "${1}" = '--lite' ]; then
				printf "\r[*] Waiting %ds for dpkg to exit..." "${_wait}"
				if [ "${_wait}" = '30' ]; then
					echo ''
					echo '[*] Waiting for dpkg process termination timed out.'
					echo '[*] Exiting itself to prevent dpkg from locking.'
					rm -f "${_dpkg_path}/status.tmp"
					rm -rf "/tmp/${_command}"
					exit 0
				fi
			else
				printf "\r[*] Waiting for dpkg to exit... waited %d seconds" "${_wait}"
			fi
			_wait="$((_wait+1))"
			sleep 1
		else
			if [ "${_wait}" -ge '2' ]; then
				echo ''
			fi
			break
		fi
		unset _lsof_dpkg
	done
	unset _lsof_dpkg _wait _w
}

_run(){
	if [ -n "${_package_row}" ]; then
		case "${_lack}" in
			description)
				_fill="Description: An awesome MobileSubstrate tweak"\!
			;;
			maintainer)
				_fill="Maintainer: someone"
			;;
			*)
				break
			;;
		esac
		if [ -n "${_fill}" ]; then
			sed -i.tmp "${_package_row}a ${_fill}" "${_dpkg_path}/status"
		fi
	fi
	unset _package _package_row _lack _fill
}

_check_dpkg_lock "${1}"
i='0'
i_max="$(dpkg -S / 2>&1 | grep -E 'escription|aintainer' | wc -l)"
if [ "${i_max}" -eq '0' ]; then
	if [ ! "${1}" = '--lite' ]; then
		echo '[*] No warning found, now exit...'
	fi
else
	while [ "${i}" -le "${i_max}" ]; do
		case "$(dpkg -S / 2>&1 | grep -E 'warning|escription|aintainer' | sed -n '2p')" in
			*escription*)
				_lack='description'
				;;
			*aintainer*)
				_lack='maintainer'
				;;
			*)
				echo ''
				echo "[*] Added missing information for \"${i}\" package"
				break
				;;
		esac
		_package="$(dpkg -S / 2>&1 | grep -E 'warning|escription|aintainer' | sed -n '1p')"
		_package="${_package%\'*}"
		_package="${_package##*\'}"
		_package_row="$(sed -n "/^Package: ${_package}$/=" "${_dpkg_path}/status")"
		if [ ! "${1}" = '--lite' ]; then
			echo "[*] found: \"${_package}\" lack: \"${_lack}\""
		else
			case "${_lack}" in
				description)
					echo "[*] fill-d: \"${_package}\""
					;;
				maintainer)
					echo "[*] fill-m: \"${_package}\""
					;;
			esac
		fi
		_check_dpkg_lock "${1}"
		_run
		i="$((i+1))"
	done
fi
rm -f "${_dpkg_path}/status.tmp"
rm -rf "/tmp/${_command}"
