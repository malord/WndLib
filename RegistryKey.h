//
// WndLib
// Copyright (c) 1994-2013 Mark H. P. Lord. All rights reserved.
//
// See LICENSE.txt for license.
//

#ifndef WNDLIB_REGISTRYKEY_H
#define WNDLIB_REGISTRYKEY_H

#include "WndLib.h"

namespace WndLib
{
	//
	// RegistryKey: A wrapper around Windows' registry API.
	//

	class WNDLIB_EXPORT RegistryKey
	{
	public:

		// Create a RegistryKey object that doesn't have a key.
		RegistryKey();

		// Create a RegistryKey object and open the specified key.
		explicit RegistryKey(HKEY root, LPCTSTR subkey = NULL);

		RegistryKey(const RegistryKey &copy);

		~RegistryKey();

		RegistryKey &operator = (const RegistryKey &copy);

		// Open a key. Returns false on failure.
		bool Open(HKEY root, LPCTSTR subkey = NULL);

		// Returns a new RegistryKey object that opens a subkey of this key.
		RegistryKey Open(LPCTSTR subkey) const;

		// Returns true if we have a key.
		bool IsOpen() const
		{
			return _refcount != NULL;
		}

		// Returns true if we don't have a key.
		bool operator ! () const
		{
			return _refcount == NULL;
		}

		// Close the key.
		void Close();

		// Read the value of a key. See RegQueryValueEx.
		bool QueryValue(LPCTSTR subkey, LPCTSTR value, DWORD *typeout, void *buffer, DWORD buffersize, DWORD *sizeout) const;

		// Read the value of this key. See RegQueryValueEx.
		bool QueryValue(LPCTSTR value, DWORD *typeout, void *buffer, DWORD buffersize, DWORD *sizeout) const;

		// Read the value of a key. See RegQueryValueEx.
		void *QueryValue(LPCTSTR subkey, LPCTSTR value, DWORD *typeout, ByteArray *buffer) const;

		// Read the value of this key. See RegQueryValueEx.
		void *QueryValue(LPCTSTR value, DWORD *typeout, ByteArray *buffer) const;

		// Read a REG_SZ or REG_EXPAND_SZ from the key.
		LPCTSTR GetString(LPCTSTR subkey, LPCTSTR value, ByteArray *buffer) const;

		// Read a REG_SZ or REG_EXPAND_SZ from the key.
		LPCTSTR GetString(LPCTSTR value, ByteArray *buffer) const;

		// Set the value of a key. See RegSetValueEx.
		bool SetValue(LPCTSTR subkey, LPCTSTR value, DWORD type, const BYTE *data, DWORD datasize);

		// Set the value of this key. See RegSetValueEx.
		bool SetValue(LPCTSTR value, DWORD type, const BYTE *data, DWORD datasize);

		// Set a string as the value of a key.
		bool SetString(LPCTSTR subkey, LPCTSTR value, LPCTSTR string, DWORD type = REG_SZ);

		// Set a string as the value of this key.
		bool SetString(LPCTSTR value, LPCTSTR string, DWORD type = REG_SZ);

		// Delete a key. "subkey" cannot be null.
		bool DeleteKey(LPCTSTR subkey);

		// Delete a value. "subkey" cannot be null.
		bool DeleteValue(LPCTSTR subkey);

		// Create a key. "subkey" cannot be null.
		RegistryKey CreateKey(LPCTSTR subkey);

		// Enumerate the contents of a key.
		bool EnumKey(LPCTSTR subkey, DWORD index, LPCTSTR *nameout, ByteArray *nameBuffer, LPCTSTR *classout = NULL, ByteArray *classBuffer = NULL);

		// Enumerate the contents of this key.
		bool EnumKey(DWORD index, LPCTSTR *nameout, ByteArray *nameBuffer, LPCTSTR *classout = NULL, ByteArray *classBuffer = NULL);

		// Enumerate the values of a key.
		bool EnumValue(LPCTSTR subkey, DWORD index, LPCTSTR *nameout, ByteArray *nameBuffer, DWORD *typeout = NULL);

		// Enumerate the values of this key.
		bool EnumValue(DWORD index, LPCTSTR *nameout, ByteArray *nameBuffer, DWORD *typeout = NULL);

	private:

		HKEY _key;
		#if WINVER < 0x0500
			LONG *_refcount;
		#else
			volatile LONG *_refcount;
		#endif
	};
}

#endif
