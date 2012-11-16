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

	void *RegistryKey::QueryValue(LPCTSTR subkey, LPCTSTR value, DWORD *typeout, Bytes *buffer) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.QueryValue(value, typeout, buffer);
	}

	void *RegistryKey::QueryValue(LPCTSTR value, DWORD *typeout, Bytes *buffer) const
	{
		DWORD size;
		if (! QueryValue(value, typeout, NULL, 0, &size))
			return NULL;

		DWORD sizeWithNull = (DWORD) (size + sizeof(TCHAR));

		void *alloced = buffer->Realloc(sizeWithNull);
		if (! QueryValue(value, typeout, alloced, sizeWithNull, &size))
			return NULL;

		((TCHAR *) alloced)[size] = 0;

		return alloced;
	}

	LPCTSTR RegistryKey::GetString(LPCTSTR subkey, LPCTSTR value, Bytes *buffer) const
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.GetString(value, buffer);
	}

	LPCTSTR RegistryKey::GetString(LPCTSTR value, Bytes *buffer) const
	{
		DWORD type;
		void *got = QueryValue(value, &type, buffer);

		if (! got)
			return NULL;

		if (type != REG_SZ && type != REG_EXPAND_SZ)
			return NULL;

		return (LPCTSTR) got;
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
		return RegDeleteKey(_key, subkey) == ERROR_SUCCESS;
	}

	RegistryKey RegistryKey::CreateKey(LPCTSTR subkey)
	{
		WNDLIB_ASSERT(IsOpen());

		HKEY result;

		if (RegCreateKeyEx(_key, subkey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &result, NULL) != ERROR_SUCCESS)
			return RegistryKey();

		return RegistryKey(result, NULL);
	}

	bool RegistryKey::EnumKey(LPCTSTR subkey, DWORD index, LPCTSTR *nameout, Bytes *nameBuffer, LPCTSTR *classout, Bytes *classBuffer)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.EnumKey(index, nameout, nameBuffer, classout, classBuffer);
	}

	bool RegistryKey::EnumKey(DWORD index, LPCTSTR *nameout, Bytes *nameBuffer, LPCTSTR *classout, Bytes *classBuffer)
	{
		TCHAR namebuf[256];
		TCHAR classbuf[64];

		DWORD nameSize = (DWORD) WNDLIB_COUNTOF(namebuf);
		DWORD classSize = (DWORD) WNDLIB_COUNTOF(classbuf);

		LONG result = RegEnumKeyEx(_key, index, namebuf, &nameSize, NULL,
			classbuf, &classSize, NULL);

		if (result == ERROR_SUCCESS)
		{
			*nameout = (LPCTSTR) nameBuffer->Realloc((nameSize + 1) * sizeof(TCHAR));
			lstrcpy((LPTSTR) *nameout, namebuf);

			if (classout)
			{
				WNDLIB_ASSERT(classBuffer);
				*classout = (LPCTSTR)
					classBuffer->Realloc((classSize + 1) * sizeof(TCHAR));
				lstrcpy((LPTSTR) *classout, classbuf);
			}

			return true;
		}

		while (result == ERROR_MORE_DATA)
		{
			nameSize += 256;
			*nameout = (LPCTSTR) nameBuffer->Realloc(nameSize * sizeof(TCHAR));

			if (classout)
			{
				WNDLIB_ASSERT(classBuffer);
				classSize += 256;
				*classout =
					(LPCTSTR) classBuffer->Realloc(classSize * sizeof(TCHAR));
			}

			result = RegEnumKeyEx(_key, index, (LPTSTR) *nameout, &nameSize, NULL, classout ? (LPTSTR) *classout : NULL, classout ? &classSize : NULL, NULL);

			if (result == ERROR_SUCCESS)
			{
				*nameout = (LPCTSTR) nameBuffer->Realloc((nameSize + 1) * sizeof(TCHAR));

				if (classout)
				{
					*classout = (LPCTSTR)
						classBuffer->Realloc((classSize + 1) * sizeof(TCHAR));
				}

				return true;
			}
		}

		return false;
	}

	bool RegistryKey::EnumValue(LPCTSTR subkey, DWORD index, LPCTSTR *nameout, Bytes *nameBuffer, DWORD *typeout)
	{
		RegistryKey sub = Open(subkey);
		if (! sub)
			return false;

		return sub.EnumValue(index, nameout, nameBuffer, typeout);
	}

	bool RegistryKey::EnumValue(DWORD index, LPCTSTR *nameout, Bytes *nameBuffer, DWORD *typeout)
	{
		TCHAR namebuf[256];
		DWORD type;

		DWORD nameSize = (DWORD) WNDLIB_COUNTOF(namebuf);

		LONG result = RegEnumValue(_key, index, namebuf, &nameSize, NULL, &type, NULL, NULL);

		if (result == ERROR_SUCCESS)
		{
			++nameSize;
			*nameout = (LPCTSTR) nameBuffer->Realloc(nameSize * sizeof(TCHAR));
			lstrcpy((LPTSTR) *nameout, namebuf);

			if (typeout)
				*typeout = type;

			return true;
		}

		while (result == ERROR_MORE_DATA)
		{
			nameSize += 32;
			*nameout = (LPCTSTR) nameBuffer->Realloc(nameSize * sizeof(TCHAR));

			result = RegEnumValue(_key, index, (LPTSTR) *nameout, &nameSize, NULL, &type, NULL, NULL);

			if (result == ERROR_SUCCESS)
				return true;
		}

		return false;
	}
}
