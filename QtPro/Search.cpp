
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include "DebuggedProcess.h"
#include "Modules.h"
#include "MemRead.h"
#include "Search.h"
#define CPPOUT fout
#define IS_IN_SEARCH(mb, offset) (mb->searchmask[(offset)/8] & (1<<((offset)%8)))
#define REMOVE_FROM_SEARCH(mb, offset)  mb->searchmask[(offset)/8] &= ~(1<<((offset)%8));


inline MEMBLOCK* create_memblock(HANDLE hProc, MEMORY_BASIC_INFORMATION *meminfo, int data_size)
{
	MEMBLOCK *mb = new MEMBLOCK; //reinterpret_cast <MEMBLOCK *>(malloc(sizeof(MEMBLOCK)));
	if (mb)
	{
		mb->hProc = hProc;
		mb->addr = reinterpret_cast <unsigned char *>(meminfo->BaseAddress);
		mb->size = meminfo->RegionSize;
		mb->buffer = reinterpret_cast <unsigned char *>(malloc(meminfo->RegionSize));
		mb->searchmask = reinterpret_cast <unsigned char *>(malloc(meminfo->RegionSize / 8));
		memset(mb->searchmask, 0xff, meminfo->RegionSize / 8);
		mb->matches = meminfo->RegionSize;
		mb->data_size = data_size;
		mb->next = NULL;
	}
	return mb;
}

void free_memblock(MEMBLOCK *mb)
{
	if (mb)
	{
		if (mb->buffer)
		{
			free(mb->buffer);
		}

		if (mb->searchmask)
		{
			free(mb->searchmask);
		}

		free(mb);
	}
}
template <typename T>
inline void update_memblock(SearchWindow* cSearchWindint, MEMBLOCK *mb, SEARCH_CONDITION condition, T val1)
{
	T val = val1;
	uint32_t nOffsetUnit = cSearchWindint->pScanOptions->ScanOffset;
	CPPOUT << "val : " << val1 << " size : " << mb->data_size << std::endl;
	/*
	if (mb->data_size == 1)
	{
		val = val1 | 0xFFFFFF00;
		CPPOUT << "valmidle : " << val << std::endl;
		val = (val ^ 0xFFFFFF00);
	}
		CPPOUT << "val : " << val << std::endl;*/
	static unsigned char tempbuf[128 * 1024];
	unsigned int bytes_left;
	unsigned int total_read;
	unsigned int bytes_to_read;
	uint64_t bytes_read;

	if (mb->matches > 0)
	{

		bytes_left = mb->size;
		total_read = 0;
		mb->matches = 0;

		while (bytes_left)
		{
			bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
			ReadProcessMemory(mb->hProc, mb->addr + total_read, tempbuf, bytes_to_read, &bytes_read);
			if (bytes_read != bytes_to_read) 
				break;
			if (condition == COND_UNCONDITIONAL)
			{
				memset(mb->searchmask + (total_read / 8), 0xff, bytes_read / 8);
				mb->matches += bytes_read;
			}
			else
			{
				unsigned int offset;
				for (offset = 0; offset < bytes_read; offset += nOffsetUnit)//mb->data_size)
				{
					if (IS_IN_SEARCH(mb, (total_read + offset)))
					{
						BOOL is_match = FALSE;
						T temp_val;
						T prev_val = 0;

						switch (mb->data_size)
						{
						case 1:
							temp_val = tempbuf[offset];
							prev_val = *((unsigned char*)&mb->buffer[total_read + offset]);
							break;
						case 2:
							temp_val = *((unsigned short*)&tempbuf[offset]);
							prev_val = *((unsigned short*)&mb->buffer[total_read + offset]);
							break;
						case 4:
							temp_val = *((unsigned int*)&tempbuf[offset]);
							prev_val = *((unsigned int*)&mb->buffer[total_read + offset]);
							break;
						case 8:
							temp_val = *((uint64_t*)&tempbuf[offset]);
							prev_val = *((uint64_t*)&mb->buffer[total_read + offset]);
							break;
						default:
							QMessageBox::warning(cSearchWindint, "Error", "This data size is not yet supported", QMessageBox::Ok);
							break;
						}

						switch (condition)
						{
						case COND_EQUALS:
							is_match = (temp_val == val);
							break;
						case COND_INCREASED:
							is_match = (temp_val > prev_val);
							break;
						case COND_DECREASED:
							is_match = (temp_val < prev_val);
							break;
						default:
							break;
						}

						if (is_match)
						{
							mb->matches++;
						}
						else
						{
							REMOVE_FROM_SEARCH(mb, (total_read + offset));
						}

					}
				}
			}
			memcpy(mb->buffer + total_read, tempbuf, bytes_read); //try memmove for optimisation
			bytes_left -= bytes_read;
			total_read += bytes_read;
		}

		mb->size = total_read;
	}
}
void update_memblock(MEMBLOCK *mb, SEARCH_CONDITION condition, float val)
{
	static unsigned char tempbuf[128 * 1024];
	unsigned int bytes_left;
	unsigned int total_read;
	unsigned int bytes_to_read;
	uint64_t bytes_read;

	if (mb->matches > 0)
	{

		bytes_left = mb->size;
		total_read = 0;
		mb->matches = 0;

		while (bytes_left)
		{
			bytes_to_read = (bytes_left > sizeof(tempbuf)) ? sizeof(tempbuf) : bytes_left;
			ReadProcessMemory(mb->hProc, mb->addr + total_read, tempbuf, bytes_to_read, &bytes_read);
			if (bytes_read != bytes_to_read) break;

			if (condition == COND_UNCONDITIONAL)
			{
				memset(mb->searchmask + (total_read / 8), 0xff, bytes_read / 8);
				mb->matches += bytes_read;
			}
			else
			{
				unsigned int offset;

				for (offset = 0; offset < bytes_read; offset += mb->data_size)
				{
					if (IS_IN_SEARCH(mb, (total_read + offset)))
					{
						BOOL is_match = FALSE;
						float temp_val;
						float prev_val = 0;

						switch (mb->data_size)
						{
						case 1:
							temp_val = tempbuf[offset];
							prev_val = *((unsigned char*)&mb->buffer[total_read + offset]);
							break;
						case 2:
							temp_val = *((unsigned short*)&tempbuf[offset]);
							prev_val = *((unsigned short*)&mb->buffer[total_read + offset]);
							break;
						case 4:
						default:
							temp_val = *((unsigned int*)&tempbuf[offset]);
							prev_val = *((unsigned int*)&mb->buffer[total_read + offset]);
							break;
						}

						switch (condition)
						{
						case COND_EQUALS:
							is_match = (temp_val == val);
							break;
						case COND_INCREASED:
							is_match = (temp_val > prev_val);
							break;
						case COND_DECREASED:
							is_match = (temp_val < prev_val);
							break;
						default:
							break;
						}

						if (is_match)
						{
							mb->matches++;
						}
						else
						{
							REMOVE_FROM_SEARCH(mb, (total_read + offset));
						}

					}
				}
			}

			memcpy(mb->buffer + total_read, tempbuf, bytes_read);

			bytes_left -= bytes_read;
			total_read += bytes_read;
		}

		mb->size = total_read;
	}
}

inline MEMBLOCK* create_scan(SearchWindow* cSearchWindint, int data_size)
{
	MEMBLOCK *mb_list = NULL;
	MEMORY_BASIC_INFORMATION meminfo;
	unsigned char *addr = 0;

	HANDLE hProc = DebuggedProc.hwnd;

	if (hProc)
	{
		while (1)
		{
			if (VirtualQueryEx(hProc, addr, &meminfo, sizeof(meminfo)) == 0)
			{
				break;
			}

			//#define WRITABLE (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)

			//if ((meminfo.State & MEM_COMMIT) && (meminfo.Protect & WRITABLE)) original code
			if (cSearchWindint->FilterRegions(meminfo))
			{

				MEMBLOCK *mb = create_memblock(hProc, &meminfo, data_size);
				if (mb)
				{
					//           update_memblock (mb);
					mb->next = mb_list;
					mb_list = mb;
				}
			}
			addr = (unsigned char*)meminfo.BaseAddress + meminfo.RegionSize;
		}
	}

	return mb_list;
}

void free_scan(MEMBLOCK *mb_list)
{
	CloseHandle(mb_list->hProc);

	while (mb_list)
	{
		MEMBLOCK *mb = mb_list;
		mb_list = mb_list->next;
		free_memblock(mb);
	}
}

template <typename T>
void update_scan(SearchWindow* cSearchWindint, MEMBLOCK *mb_list, SEARCH_CONDITION condition, T val)
{
	MEMBLOCK *mb = mb_list;
	while (mb)
	{
		update_memblock(cSearchWindint, mb, condition, val);
		mb = mb->next;
	}
}

void dump_scan_info(MEMBLOCK *mb_list)
{
	MEMBLOCK *mb = mb_list;

	while (mb)
	{
		int i;
		printf("0x%08x %d\r\n", mb->addr, mb->size);

		for (i = 0; i < mb->size; i++)
		{
			printf("%02x", mb->buffer[i]);
		}
		printf("\r\n");

		mb = mb->next;
	}
}

void poke(HANDLE hProc, int data_size, unsigned int addr, unsigned int val)
{
	if (WriteProcessMemory(hProc, (void*)addr, &val, data_size, NULL) == 0)
	{
		printf("poke failed\r\n");
	}
}

unsigned int peek(HANDLE hProc, int data_size, unsigned int addr)
{
	unsigned int val = 0;

	if (ReadProcessMemory(hProc, (void*)addr, &val, data_size, NULL) == 0)
	{
		printf("poke failed\r\n");
	}

	return val;
}

void print_matches(MEMBLOCK *mb_list, Ui_DialogResults* pResultWindow, SearchWindow* pSearchWindow)
{
	unsigned int offset;
	MEMBLOCK *mb = mb_list;
	std::string str;
	while (mb)
	{
		for (offset = 0; offset < mb->size; offset += pSearchWindow->pScanOptions->ScanOffset)//mb->data_size)
		{
			if (IS_IN_SEARCH(mb, offset))
			{
				uint64_t val = peek(mb->hProc, mb->data_size, (unsigned int)mb->addr + offset);
				str = pSearchWindow->ModMap->FetchModuleName(reinterpret_cast<int64_t>(mb->addr + offset));
				if (str != "unknown")
				{
					QTreeWidgetItem * itm = new QTreeWidgetItem(pResultWindow->treeWidget);
					itm->setText(0, ReturnStrFromHexaInt((int64_t)mb->addr + offset).c_str());
					itm->setTextColor(0, Qt::darkGreen);
					itm->setText(1, ReturnStrFromHexaInt(val).c_str());
				}
				else
				{
					QTreeWidgetItem * itm = new QTreeWidgetItem(pResultWindow->treeWidget);
					itm->setText(0, ReturnStrFromHexaInt((int64_t)mb->addr + offset).c_str());
					itm->setText(1, ReturnStrFromHexaInt(val).c_str());
				}
			}
		}

		mb = mb->next;
	}
}

int get_match_count(MEMBLOCK *mb_list)
{
	MEMBLOCK *mb = mb_list;
	int count = 0;

	while (mb)
	{
		count += mb->matches;
		mb = mb->next;
	}

	return count;
}

unsigned int str2int(char *s)
{
	int base = 10;

	if (s[0] == '0' && s[1] == 'x')
	{
		base = 16;
		s += 2;
	}

	return strtoul(s, NULL, base);
}
template <typename T>
MEMBLOCK* ui_new_scan(SearchWindow* cSearchWindint, uint32_t data_size, T start_val, SEARCH_CONDITION start_cond)
{
	MEMBLOCK *scan = NULL;
	while (1)
	{
		scan = create_scan(cSearchWindint, data_size);
		if (scan) break;
		QMessageBox::warning(cSearchWindint, "Error", "Invalid Scan", QMessageBox::Ok);
	}
	update_scan(cSearchWindint, scan, start_cond, start_val);
	return scan;
}

void ui_poke(HANDLE hProc, int data_size)
{
	unsigned int addr;
	unsigned int val;
	char s[20];

	printf("Enter the address: ");
	fgets(s, sizeof(s), stdin);
	addr = str2int(s);

	printf("\r\nEnter the value: ");
	fgets(s, sizeof(s), stdin);
	val = str2int(s);
	printf("\r\n");

	poke(hProc, data_size, addr, val);
}

template <typename T>
int SearchWindow::ui_run_scan(MEMBLOCK *scan1, uint32_t data_size, T start_val, SEARCH_CONDITION start_cond, SCAN_CONDITION scan_condition)
{
	scan = DebuggedProc.mb;
	unsigned int val;
	char s[20];
	switch (scan_condition)
	{
	case NEW_SCAN:
		//if (scan);
		//free_scan(scan);
		DebuggedProc.mb = ui_new_scan(this, data_size, start_val, start_cond);
		CPPOUT << "NEW_SCAN, matches found " << get_match_count(DebuggedProc.mb) << " start condition : " << scan_condition << " start val : " << std::dec << start_val << " data size : " << data_size << std::endl;
		break;
	case NEXT_SCAN:
		switch (start_cond)
		{
		case COND_INCREASED:
			update_scan(this, scan, COND_INCREASED, start_val);
			CPPOUT << "NEXT_SCANinc, matches found " << get_match_count(scan) << " start condition : " << scan_condition << " start val : " << std::dec << " data size : " << data_size << std::endl;
			break;
		case COND_DECREASED:
			update_scan(this, scan, COND_DECREASED, start_val);
			CPPOUT << "NEXT_SCANdec, matches found " << get_match_count(scan) << " start condition : " << scan_condition << " start val : " << std::dec << start_val << " data size : " << data_size << std::endl;
			break;
		case COND_EQUALS:
			update_scan(this, scan, COND_EQUALS, start_val);
			CPPOUT << "NEXT_SCANeq, matches found " << get_match_count(scan) << " start condition : " << scan_condition << " start val : " << std::dec << start_val << " data size : " << data_size << std::endl;
			break;
		}
		break;
	}
	return get_match_count(DebuggedProc.mb);
	while (1)
	{
		printf("\r\nEnter the next value or");
		printf("\r\n[i] increased");
		printf("\r\n[d] decreased");
		printf("\r\n[m] print matches");
		printf("\r\n[p] poke address");
		printf("\r\n[n] new scan");
		printf("\r\n[q] quit\r\n");

		fgets(s, sizeof(s), stdin);
		printf("\r\n");

		switch (s[0])
		{
		case 'i':
			update_scan(this, scan, COND_INCREASED, 0);
			CPPOUT << "  matches found " << get_match_count(scan) << std::endl;
			break;
		case 'd':
			update_scan(this, scan, COND_DECREASED, 0);
			CPPOUT << "  matches found " << get_match_count(scan) << std::endl;
			break;
		case 'm':
			//print_matches(scan);
			break;
		case 'p':
			ui_poke(scan->hProc, scan->data_size);
			break;
		case 'n':
			free_scan(scan);
			scan = ui_new_scan(this, data_size, start_val, start_cond);
			break;
		case 'q':
			free_scan(scan);
			return get_match_count(scan);
		default:
			val = str2int(s);
			update_scan(this, scan, COND_EQUALS, val);
			CPPOUT << "  matches found " << get_match_count(scan) << std::endl;
			break;
		}
	}
}
uint32_t SearchWindow::ReturnDataSize()
{
	switch (static_cast<DATA_SIZE>(ui.comboBValueType->currentIndex()))
	{
	case DATA_SIZE::BYTE_:
		return 1;
	case DATA_SIZE::TWOBYTES_:
		return 2;
	case DATA_SIZE::INT32_:
		return 4;
	case DATA_SIZE::INT64_:
		return 8;
	case DATA_SIZE::FLOAT_:
		return  sizeof(float);
	case DATA_SIZE::DOUBLE_:
		return sizeof(double);
	case DATA_SIZE::STRING_:
		return 0;
	}
}
template <typename T>
void CompareFn(T& lhs, T& rhs, int compare)
{
	bool result = false;
	switch (compare)
	{
	case 1:
		result = lhs > rhs;
		break;
	case 2:
		result = lhs < rhs;
		break;
	}
}
void ScanParameterBase::UpdateSelectedScanOptions(SearchWindow * pSearchWindow) {
	this->CurrentScanFastScan = pSearchWindow->ui.cbFastScan->isChecked();
	this->GlobalScanType = static_cast <SEARCH_CONDITION> (pSearchWindow->ui.comboBScanType->currentIndex());//Better performance matching ints than comparing strings
	this->GlobalScanValueType = static_cast<DATA_SIZE> (pSearchWindow->ui.comboBValueType->currentIndex());
	this->CurrentScanHexValues = pSearchWindow->ui.cbHex->isChecked();
	this->ValueSize = pSearchWindow->ReturnDataSize();
	this->AcceptedMemoryState;//unfinished
	if (this->CurrentScanFastScan)
		this->ScanOffset = this->ValueSize;
	else
		this->ScanOffset = 1;

	CPPOUT << this->GlobalScanType << std::endl;
}
//defined here for compilation reasons
void ScanParameterBase::GetValue(SearchWindow * pSearchWindow, SCAN_CONDITION NewOrNext)
{
	float nFloat;
	double dDouble;
	QString TextValue = pSearchWindow->ui.LineScanValue->text();
	switch (this->GlobalScanValueType)
	{
	case 1:
		if (this->CurrentScanHexValues)
			nValue8 = TextValue.toInt(0, 16);
		else
			nValue8 = TextValue.toInt(0, 10);
		pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue8, this->GlobalScanType, NewOrNext);
		break;
	case 2:
		if (this->CurrentScanHexValues)
			nValue16 = TextValue.toInt(0, 16);
		else
			nValue16 = TextValue.toInt(0, 10);
		pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue16, this->GlobalScanType, NewOrNext);
		break;
	case 3:
		if (this->CurrentScanHexValues)
			nValue32 = TextValue.toInt(0, 16);
		else
			nValue32 = TextValue.toInt(0, 10);
		pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue32, this->GlobalScanType, NewOrNext);
		break;
	case 4:
		if (this->CurrentScanHexValues)
			nValue64 = TextValue.toLongLong(0, 16);
		else
			nValue64 = TextValue.toLongLong(0, 10);
		pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue64, this->GlobalScanType, NewOrNext);
		break;
	case 5://float case, no hex float
			nFloat = TextValue.toFloat();
			nValue32 = *(reinterpret_cast<int*>(&nFloat));
			pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue32, this->GlobalScanType, NewOrNext);
		break;
	case 6:
		dDouble = TextValue.toDouble();
		nValue64 = *(reinterpret_cast<int64_t*>(&dDouble));
		pSearchWindow->nResults = pSearchWindow->ui_run_scan(DebuggedProc.mb, this->ValueSize, this->nValue64, this->GlobalScanType, NewOrNext);
		break;
	default:
		QMessageBox::warning(pSearchWindow, "Error", "This value type is not supported yet", QMessageBox::Ok);
		break;
	}
}