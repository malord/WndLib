#include "RegistryKey.h"

namespace WndLib
{
	RegistryKey::RegistryKey()
	{
		_key = NULL;
		_refcount = NULL;
	}

	RegistryKey::RegistryKey(HKEY root, LPCTSTR subkey)
	{
		_refcount = NULL;
		Open(root, subkey);
	}

	RegistryKey::RegistryKey(const RegistryKey &copy)
	{
		_refcount = NULL;
		operator = (copy);
	}

	RegistryKey &RegistryKey::operator = (const RegistryKey &copy)
	{
		if (this != &copy)
		{
			if (copy._refcount)
				InterlockedIncrement(copy._refcount);

			Close();

			_key = copy._key;
			_refcount = copy._refcount;
		}

		return *this;
	}

	RegistryKey::~RegistryKey()
	{
		Close();
	}

	bool RegistryKey::Open(HKEY root, LPCTSTR subkey)
	{
		Close();

		if (RegOpenKey(root, subkey, &_key) != ERROR_SUCCESS)
			return false;

		_refcount = new LONG(1);
		return true;
	}

	RegistryKey RegistryKey::Open(LPCTSTR subkey) const
	{
		WNDLIB_ASSERT(IsOpen());

		RegistryKey object;
		object.Open(_key, subkey);

		return object;
	}

	void RegistryKey::Close()
	{
		if (_refcount)
		{
			if (! InterlockedDecrement(_refcount))
			{
				RegCloseKey(_key);
				delete _refcount;
			}

			_refcount = NULL;
		}
	}

	bool RegistryKey::QueryValue(LPCTSTR subkey, LPCTSTR value, DWORD *typeout, void *buffer, DWORD buffersize, DWORD *sizeout) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.QueryValue(value, typeout, buffer, buffersize, sizeout);
	}

	bool RegistryKey::QueryValue(LPCTSTR value, DWORD *typeout, void *buffer, DWORD buffersize, DWORD *sizeout) const
	{
		*sizeout = buffersize;

		if (RegQueryValueEx(_key, value, NULL, typeout, (LPBYTE) buffer,
			sizeout) != ERROR_SUCCESS)
		{
			return false;
		}

		return true;
	}

	void *RegistryKey::QueryValue(LPCTSTR subkey, LPCTSTR value, DWORD *typeout, TCharString *buffer) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.QueryValue(value, typeout, buffer);
	}

	void *RegistryKey::QueryValue(LPCTSTR value, DWORD *typeout, TCharString *buffer) const
	{
		DWORD size;
		if (! QueryValue(value, typeout, NULL, 0, &size))
			return NULL;

		DWORD sizeWithNull = (DWORD) (size + sizeof(TCHAR));

		buffer->resize((sizeWithNull + sizeof(TCHAR) / 2) / sizeof(TCHAR));
		void *alloced = &(*buffer)[0];
		if (! QueryValue(value, typeout, alloced, sizeWithNull, &size))
			return NULL;

		buffer->resize((size + sizeof(TCHAR) / 2) / sizeof(TCHAR));

		switch (*typeout) 
		{
			case REG_SZ:
			case REG_EXPAND_SZ:
				// RegQueryValueEx may or may not return a null terminated 
				// string depending on whether RegSetValue was called
				// correctly. Assume RegSetValue was set correctly, and
				// pop off a trailing null terminator.
				if (! buffer->empty() && (*buffer)[buffer->size() - 1] == 0)
					buffer->resize(buffer->size() - 1);
				break;
			default:
				break;
		}

		return alloced;
	}

	DWORD RegistryKey::GetDWORD(LPCTSTR subkey, LPCTSTR value, DWORD errorValue) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.GetDWORD(value, errorValue);
	}

	DWORD RegistryKey::GetDWORD(LPCTSTR value, DWORD errorValue) const
	{
		TCharString buffer;
		DWORD type;
		void *got = QueryValue(value, &type, &buffer);

		if (! got)
			return errorValue;

		if (type != REG_DWORD)
			return errorValue;

		return *(LPDWORD) got;
	}

	bool RegistryKey::SetDWORD(LPCTSTR subkey, LPCTSTR value, DWORD number)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.SetDWORD(value, number);
	}

	bool RegistryKey::SetDWORD(LPCTSTR value, DWORD number)
	{
		return SetValue(value, REG_DWORD, (const BYTE *) &number, sizeof(DWORD));
	}

	LPCTSTR RegistryKey::GetString(LPCTSTR subkey, LPCTSTR value, TCharString *buffer) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.GetString(value, buffer);
	}

	LPCTSTR RegistryKey::GetString(LPCTSTR value, TCharString *buffer) const
	{
		DWORD type;
		void *got = QueryValue(value, &type, buffer);

		if (! got)
			return NULL;

		// TODO: expand REG_EXPAND_SZ?

		if (type != REG_SZ && type != REG_EXPAND_SZ)
			return NULL;

		return (LPCTSTR) got;
	}

	TCharString RegistryKey::GetString(LPCTSTR subkey, LPCTSTR value) const
	{
		TCharString string;
		if (! GetString(subkey, value, &string))
			string.resize(0);
		return string;
	}

	TCharString RegistryKey::GetString(LPCTSTR value) const
	{
		TCharString string;
		if (! GetString(value, &string))
			string.resize(0);
		return string;
	}

	bool RegistryKey::SetValue(LPCTSTR subkey, LPCTSTR value, DWORD type, const BYTE *data, DWORD datasize)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.SetValue(value, type, data, datasize);
	}

	bool RegistryKey::SetValue(LPCTSTR value, DWORD type, const BYTE *data, DWORD datasize)
	{
		WNDLIB_ASSERT(IsOpen());

		if (RegSetValueEx(_key, value, NULL, type, data, datasize) != ERROR_SUCCESS)
			return false;

		return true;
	}

	bool RegistryKey::SetString(LPCTSTR subkey, LPCTSTR value, LPCTSTR string, DWORD type)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.SetString(value, string, type);
	}

	bool RegistryKey::SetString(LPCTSTR value, LPCTSTR string, DWORD type)
	{
		return SetValue(value, type, (const BYTE *) string, (lstrlen(string) + 1) * sizeof(TCHAR));
	}

	bool RegistryKey::DeleteKey(LPCTSTR subkey)
	{
		WNDLIB_ASSERT(IsOpen());
		LONG result = RegDeleteKey(_key, subkey);
		return result == ERROR_SUCCESS;
	}

	bool RegistryKey::DeleteValue(LPCTSTR subkey)
	{
		WNDLIB_ASSERT(IsOpen());
		LONG result = RegDeleteValue(_key, subkey);
		return result == ERROR_SUCCESS;
	}

	RegistryKey RegistryKey::CreateKey(LPCTSTR subkey)
	{
		WNDLIB_ASSERT(IsOpen());

		HKEY result;

		if (RegCreateKeyEx(_key, subkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &result, NULL) != ERROR_SUCCESS)
			return RegistryKey();

		return RegistryKey(result, NULL);
	}

	bool RegistryKey::EnumKey(LPCTSTR subkey, DWORD index, TCharString *nameout, TCharString *classout)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.EnumKey(index, nameout, classout);
	}

	bool RegistryKey::EnumKey(DWORD index, TCharString *nameout, TCharString *classout)
	{
		TCHAR namebuf[256];
		DWORD nameSize = (DWORD) WNDLIB_COUNTOF(namebuf);

		TCHAR classbuf[64];
		DWORD classSize = (DWORD) WNDLIB_COUNTOF(classbuf);

		LONG result = RegEnumKeyEx(_key, index, namebuf, &nameSize, NULL, classbuf, &classSize, NULL);

		if (result == ERROR_SUCCESS)
		{
			*nameout = namebuf;

			if (classout)
				*classout = classbuf;

			return true;
		}

		while (result == ERROR_MORE_DATA)
		{
			nameSize += 256;
			nameout->resize(nameSize);

			if (classout)
			{
				classSize += 256;
				classout->resize(classSize);
			}

			result = RegEnumKeyEx(_key, index, &(*nameout)[0], &nameSize, NULL, classout ? (LPTSTR) &(*classout)[0] : NULL, classout ? &classSize : NULL, NULL);

			if (result == ERROR_SUCCESS)
			{
				nameout->resize(lstrlen(&(*nameout)[0]));

				if (classout)
					classout->resize(lstrlen(&(*classout)[0]));

				return true;
			}
		}

		return false;
	}

	bool RegistryKey::EnumValue(LPCTSTR subkey, DWORD index, TCharString *nameout, DWORD *typeout)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.EnumValue(index, nameout, typeout);
	}

	bool RegistryKey::EnumValue(DWORD index, TCharString *nameout, DWORD *typeout)
	{
		TCHAR namebuf[256];
		DWORD nameSize = (DWORD) WNDLIB_COUNTOF(namebuf);

		DWORD type;
		LONG result = RegEnumValue(_key, index, namebuf, &nameSize, NULL, &type, NULL, NULL);

		if (result == ERROR_SUCCESS)
		{
			*nameout = namebuf;

			if (typeout)
				*typeout = type;

			return true;
		}

		while (result == ERROR_MORE_DATA)
		{
			nameSize += 128;
			nameout->resize(nameSize);

			result = RegEnumValue(_key, index, (LPTSTR) &(*nameout)[0], &nameSize, NULL, &type, NULL, NULL);

			if (result == ERROR_SUCCESS)
			{
				nameout->resize(lstrlen(&(*nameout)[0]));
				return true;
			}
		}

		return false;
	}
}
