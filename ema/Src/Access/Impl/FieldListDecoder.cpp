/*|-----------------------------------------------------------------------------
 *|            This source code is provided under the Apache 2.0 license      --
 *|  and is provided AS IS with no warranty or guarantee of fit for purpose.  --
 *|                See the project's LICENSE.md for details.                  --
 *|           Copyright Thomson Reuters 2015. All rights reserved.            --
 *|-----------------------------------------------------------------------------
 */

#include "FieldListDecoder.h"
#include "StaticDecoder.h"
#include "Encoder.h"

using namespace thomsonreuters::ema::access;

extern const EmaString& getDTypeAsString( DataType::DataTypeEnum dType );

FieldListDecoder::FieldListDecoder() :
 _rsslFieldList(),
 _rsslFieldListBuffer(),
 _rsslFieldEntry(),
 _decodeIter(),
 _load(),
 _pRsslDictionary( 0 ),
 _rsslDictionaryEntry( 0 ),
 _rsslLocalFLSetDefDb( 0 ),
 _name(),
 _hexBuffer(),
 _rsslMajVer( RSSL_RWF_MAJOR_VERSION ),
 _rsslMinVer( RSSL_RWF_MINOR_VERSION ),
 _errorCode( OmmError::NoErrorEnum ),
 _decodingStarted( false ),
 _atEnd( false )
{
	rsslClearFieldList( &_rsslFieldList );
 }

FieldListDecoder::~FieldListDecoder()
{
	StaticDecoder::morph( &_load, DataType::NoDataEnum );
}

bool FieldListDecoder::hasInfo() const
{
	return _rsslFieldList.flags & RSSL_FLF_HAS_FIELD_LIST_INFO ? true : false;
}

Int16 FieldListDecoder::getInfoFieldListNum() const
{
	if ( !hasInfo() )
	{
		EmaString temp( "Attempt to getInfoFieldListNum() while FieldList Info is NOT set." );

		throwIueException( temp );
	}

	return _rsslFieldList.fieldListNum;
}

Int16 FieldListDecoder::getInfoDictionaryId() const
{
	if ( !hasInfo() )
	{
		EmaString temp( "Attempt to getInfoDictionaryId() while FieldList Info is NOT set." );

		throwIueException( temp );
	}

	return _rsslFieldList.dictionaryId;
}

void FieldListDecoder::clone( const FieldListDecoder& other )
{
	_decodingStarted = false;

	_rsslMajVer = other._rsslMajVer;

	_rsslMinVer = other._rsslMinVer;

	_rsslFieldListBuffer = other._rsslFieldListBuffer;

	_pRsslDictionary = other._pRsslDictionary;

	_rsslLocalFLSetDefDb = other._rsslLocalFLSetDefDb;

	if ( !_pRsslDictionary )
	{
		_atEnd = false;
		_errorCode = OmmError::NoDictionaryEnum;
		return;
	}

	rsslClearDecodeIterator( &_decodeIter );

	RsslRet retCode = rsslSetDecodeIteratorBuffer( &_decodeIter, &other._rsslFieldListBuffer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslSetDecodeIteratorRWFVersion( &_decodeIter, _rsslMajVer, _rsslMinVer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslDecodeFieldList( &_decodeIter, &_rsslFieldList, _rsslLocalFLSetDefDb );

	switch ( retCode )
	{
	case RSSL_RET_NO_DATA :
		_atEnd = true;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_SUCCESS :
		_atEnd = false;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_ITERATOR_OVERRUN :
		_atEnd = false;
		_errorCode = OmmError::IteratorOverrunEnum;
		break;
	case RSSL_RET_INCOMPLETE_DATA :
		_atEnd = false;
		_errorCode = OmmError::IncompleteDataEnum;
		break;
	case RSSL_RET_SET_SKIPPED :
		_atEnd = false;
		_errorCode = OmmError::NoSetDefinitionEnum;
		break;
	default :
		_atEnd = false;
		_errorCode = OmmError::UnknownErrorEnum;
		break;
	}
}

void FieldListDecoder::setRsslData( RsslDecodeIterator* , RsslBuffer* )
{
}

void FieldListDecoder::reset()
{
	_decodingStarted = false;

	if ( !_pRsslDictionary )
	{
		_atEnd = false;
		_errorCode = OmmError::NoDictionaryEnum;
		return;
	}

	rsslClearDecodeIterator( &_decodeIter );

	RsslRet retCode = rsslSetDecodeIteratorBuffer( &_decodeIter, &_rsslFieldListBuffer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslSetDecodeIteratorRWFVersion( &_decodeIter, _rsslMajVer, _rsslMinVer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = !_rsslFieldListBuffer.length ? true : false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslDecodeFieldList( &_decodeIter, &_rsslFieldList, _rsslLocalFLSetDefDb );

	_rsslDictionaryEntry = 0;

	switch ( retCode )
	{
	case RSSL_RET_NO_DATA :
		_atEnd = true;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_SUCCESS :
		_atEnd = false;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_ITERATOR_OVERRUN :
		_atEnd = false;
		_errorCode = OmmError::IteratorOverrunEnum;
		break;
	case RSSL_RET_INCOMPLETE_DATA :
		_atEnd = false;
		_errorCode = OmmError::IncompleteDataEnum;
		break;
	case RSSL_RET_SET_SKIPPED :
		_atEnd = false;
		_errorCode = OmmError::NoSetDefinitionEnum;
		break;
	default :
		_atEnd = false;
		_errorCode = OmmError::UnknownErrorEnum;
		break;
	}
}

void FieldListDecoder::setRsslData( UInt8 , UInt8 , RsslMsg* , const RsslDataDictionary* )
{
}

void FieldListDecoder::setRsslData( UInt8 majVer, UInt8 minVer, RsslBuffer* rsslBuffer, const RsslDataDictionary* rsslDictionary, void* localFlSetDefDb )
{
	_decodingStarted = false;

	_rsslMajVer = majVer;

	_rsslMinVer = minVer;

	_rsslFieldListBuffer = *rsslBuffer;

	_pRsslDictionary = rsslDictionary;

	_rsslLocalFLSetDefDb = (RsslLocalFieldSetDefDb*)localFlSetDefDb;

	if ( !_pRsslDictionary )
	{
		_atEnd = false;
		_errorCode = OmmError::NoDictionaryEnum;
		return;
	}

	rsslClearDecodeIterator( &_decodeIter );

	RsslRet retCode = rsslSetDecodeIteratorBuffer( &_decodeIter, rsslBuffer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslSetDecodeIteratorRWFVersion( &_decodeIter, _rsslMajVer, _rsslMinVer );
	if ( RSSL_RET_SUCCESS != retCode )
	{
		_atEnd = false;
		_errorCode = OmmError::IteratorSetFailureEnum;
		return;
	}

	retCode = rsslDecodeFieldList( &_decodeIter, &_rsslFieldList, _rsslLocalFLSetDefDb );

	switch ( retCode )
	{
	case RSSL_RET_NO_DATA :
		_atEnd = true;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_SUCCESS :
		_atEnd = false;
		_errorCode = OmmError::NoErrorEnum;
		break;
	case RSSL_RET_ITERATOR_OVERRUN :
		_atEnd = false;
		_errorCode = OmmError::IteratorOverrunEnum;
		break;
	case RSSL_RET_INCOMPLETE_DATA :
		_atEnd = false;
		_errorCode = OmmError::IncompleteDataEnum;
		break;
	case RSSL_RET_SET_SKIPPED :
		_atEnd = false;
		_errorCode = OmmError::NoSetDefinitionEnum;
		break;
	default :
		_atEnd = false;
		_errorCode = OmmError::UnknownErrorEnum;
		break;
	}
}

bool FieldListDecoder::getNextData()
{
	if ( _atEnd ) return true;

	if ( !_decodingStarted && _errorCode != OmmError::NoErrorEnum )
	{
		_atEnd = true;
		_decodingStarted = true;
		Decoder::setRsslData( (&_load), _errorCode, &_decodeIter, &_rsslFieldListBuffer ); 
		return false;
	}

	_decodingStarted = true;

	RsslRet retCode = rsslDecodeFieldEntry( &_decodeIter, &_rsslFieldEntry );

	_rsslDictionaryEntry = _pRsslDictionary->entriesArray[_rsslFieldEntry.fieldId];

	switch ( retCode )
	{
	case RSSL_RET_END_OF_CONTAINER :
		_atEnd = true;
		return true;
	case RSSL_RET_SUCCESS :
		if ( _rsslDictionaryEntry )
			Decoder::setRsslData( &_load, _rsslDictionaryEntry->rwfType,
									&_decodeIter, &_rsslFieldEntry.encData, _pRsslDictionary, 0 ); 
		else
			Decoder::setRsslData( &_load, OmmError::FieldIdNotFoundEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_INCOMPLETE_DATA :
		Decoder::setRsslData( &_load, OmmError::IncompleteDataEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_UNSUPPORTED_DATA_TYPE :
		Decoder::setRsslData( &_load, OmmError::UnsupportedDataTypeEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	default :
		Decoder::setRsslData( &_load, OmmError::UnknownErrorEnum, &_decodeIter, &_rsslFieldEntry.encData );
		return false;
	}
}

bool FieldListDecoder::getNextData( Int16 fieldId )
{
	RsslRet retCode = RSSL_RET_SUCCESS;

	do {
		if ( _atEnd ) return true;

		if ( !_decodingStarted && _errorCode != OmmError::NoErrorEnum )
		{
			_atEnd = true;
			_decodingStarted = true;
			Decoder::setRsslData( &_load, _errorCode, &_decodeIter, &_rsslFieldListBuffer ); 
			return false;
		}

		_decodingStarted = true;

		retCode = rsslDecodeFieldEntry( &_decodeIter, &_rsslFieldEntry );

		if ( retCode == RSSL_RET_END_OF_CONTAINER )
		{
			_atEnd = true;
			return true;
		}
	}
	while (	_rsslFieldEntry.fieldId != fieldId );

	_rsslDictionaryEntry = _pRsslDictionary->entriesArray[_rsslFieldEntry.fieldId];

	switch ( retCode )
	{
	case RSSL_RET_SUCCESS :
		if ( _rsslDictionaryEntry )
			Decoder::setRsslData( &_load, _rsslDictionaryEntry->rwfType,
									&_decodeIter, &_rsslFieldEntry.encData, _pRsslDictionary, 0 ); 
		else
			Decoder::setRsslData( &_load, OmmError::FieldIdNotFoundEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_INCOMPLETE_DATA :
		Decoder::setRsslData( &_load, OmmError::IncompleteDataEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_UNSUPPORTED_DATA_TYPE :
		Decoder::setRsslData( &_load, OmmError::UnsupportedDataTypeEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	default :
		Decoder::setRsslData( &_load, OmmError::UnknownErrorEnum, &_decodeIter, &_rsslFieldEntry.encData );
		return false;
	}
}

bool FieldListDecoder::getNextData( const EmaString& name )
{
	RsslRet retCode = RSSL_RET_SUCCESS;
	bool matchName = false;
	EmaStringInt tempName;

	do {
		if ( _atEnd ) return true;

		if ( !_decodingStarted && _errorCode != OmmError::NoErrorEnum )
		{
			_atEnd = true;
			_decodingStarted = true;
			Decoder::setRsslData( &_load, _errorCode, &_decodeIter, &_rsslFieldListBuffer ); 
			return false;
		}

		_decodingStarted = true;

		retCode = rsslDecodeFieldEntry( &_decodeIter, &_rsslFieldEntry );

		if ( retCode == RSSL_RET_END_OF_CONTAINER )
		{
			_atEnd = true;
			return true;
		}

		_rsslDictionaryEntry = _pRsslDictionary->entriesArray[_rsslFieldEntry.fieldId];

		if ( _rsslDictionaryEntry )
		{
			tempName.setInt(  _rsslDictionaryEntry->acronym.data, _rsslDictionaryEntry->acronym.length, true );
			if ( name == tempName.toString() )
				matchName = true;
		}

	} while ( !matchName );

	switch ( retCode )
	{
	case RSSL_RET_SUCCESS :
		if ( _rsslDictionaryEntry )
			Decoder::setRsslData( &_load, _rsslDictionaryEntry->rwfType,
									&_decodeIter, &_rsslFieldEntry.encData, _pRsslDictionary, 0 ); 
		else
			Decoder::setRsslData( &_load, OmmError::FieldIdNotFoundEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_INCOMPLETE_DATA :
		Decoder::setRsslData( &_load, OmmError::IncompleteDataEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_UNSUPPORTED_DATA_TYPE :
		Decoder::setRsslData( &_load, OmmError::UnsupportedDataTypeEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	default :
		Decoder::setRsslData( &_load, OmmError::UnknownErrorEnum, &_decodeIter, &_rsslFieldEntry.encData );
		return false;
	}
}

bool FieldListDecoder::getNextData( const EmaVector< Int16 >& intList )
{
	RsslRet retCode = RSSL_RET_SUCCESS;
	bool match = false;

	do {
		if ( _atEnd ) return true;

		if ( !_decodingStarted && _errorCode != OmmError::NoErrorEnum )
		{
			_atEnd = true;
			_decodingStarted = true;
			Decoder::setRsslData( &_load, _errorCode, &_decodeIter, &_rsslFieldListBuffer ); 
			return false;
		}

		_decodingStarted = true;

		retCode = rsslDecodeFieldEntry( &_decodeIter, &_rsslFieldEntry );

		if ( retCode == RSSL_RET_END_OF_CONTAINER )
		{
			_atEnd = true;
			return true;
		}

		UInt32 size = intList.size();
		for ( UInt32 idx = 0; idx < size; ++idx )
		{
			if ( _rsslFieldEntry.fieldId == intList[idx] )
			{
				match = true;
				break;
			}
		}
	}
	while (	!match );

	_rsslDictionaryEntry = _pRsslDictionary->entriesArray[_rsslFieldEntry.fieldId];

	switch ( retCode )
	{
	case RSSL_RET_SUCCESS :
		if ( _rsslDictionaryEntry )
			Decoder::setRsslData( &_load, _rsslDictionaryEntry->rwfType,
									&_decodeIter, &_rsslFieldEntry.encData, _pRsslDictionary, 0 ); 
		else
			Decoder::setRsslData( &_load, OmmError::FieldIdNotFoundEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_INCOMPLETE_DATA :
		Decoder::setRsslData( &_load, OmmError::IncompleteDataEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_UNSUPPORTED_DATA_TYPE :
		Decoder::setRsslData( &_load, OmmError::UnsupportedDataTypeEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	default :
		Decoder::setRsslData( &_load, OmmError::UnknownErrorEnum, &_decodeIter, &_rsslFieldEntry.encData );
		return false;
	}
}

bool FieldListDecoder::getNextData( const EmaVector< EmaString >& stringList )
{
	RsslRet retCode = RSSL_RET_SUCCESS;
	bool matchName = false;
	EmaStringInt tempName;

	do {
		if ( _atEnd ) return true;

		if ( !_decodingStarted && _errorCode != OmmError::NoErrorEnum )
		{
			_atEnd = true;
			_decodingStarted = true;
			Decoder::setRsslData( &_load, _errorCode, &_decodeIter, &_rsslFieldListBuffer ); 
			return false;
		}

		_decodingStarted = true;

		retCode = rsslDecodeFieldEntry( &_decodeIter, &_rsslFieldEntry );

		if ( retCode == RSSL_RET_END_OF_CONTAINER )
		{
			_atEnd = true;
			return true;
		}

		_rsslDictionaryEntry = _pRsslDictionary->entriesArray[_rsslFieldEntry.fieldId];

		if ( _rsslDictionaryEntry )
		{
			tempName.setInt(  _rsslDictionaryEntry->acronym.data, _rsslDictionaryEntry->acronym.length, true );
			
			UInt32 size = stringList.size();
			for ( UInt32 idx = 0; idx < size; ++idx )
			{
				if ( stringList[idx] == tempName.toString() )
				{
					matchName = true;
					break;
				}
			}
		}

	} while ( !matchName );

	switch ( retCode )
	{
	case RSSL_RET_SUCCESS :
		if ( _rsslDictionaryEntry )
			Decoder::setRsslData( &_load, _rsslDictionaryEntry->rwfType,
									&_decodeIter, &_rsslFieldEntry.encData, _pRsslDictionary, 0 ); 
		else
			Decoder::setRsslData( &_load, OmmError::FieldIdNotFoundEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_INCOMPLETE_DATA :
		Decoder::setRsslData( &_load, OmmError::IncompleteDataEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	case RSSL_RET_UNSUPPORTED_DATA_TYPE :
		Decoder::setRsslData( &_load, OmmError::UnsupportedDataTypeEnum, &_decodeIter, &_rsslFieldEntry.encData ); 
		return false;
	default :
		Decoder::setRsslData( &_load, OmmError::UnknownErrorEnum, &_decodeIter, &_rsslFieldEntry.encData );
		return false;
	}
}

bool FieldListDecoder::getNextData( const Data& data )
{
	if ( data.getDataType() != DataType::ElementListEnum )
	{
		EmaString temp( "Wrong container type used for passing search list. Expecting ElementList. Received " );
		temp += getDTypeAsString( data.getDataType() );

		// todo ... logger output to be provided

		_atEnd = true;
		return true;
	}

	RsslDataType listDataType = RSSL_DT_NO_DATA;
	EmaVector< Int16 > intList;
	EmaVector< EmaString > stringList;	

	if ( !decodeViewList( &data.getEncoder().getRsslBuffer(), listDataType, intList, stringList ) )
	{
		_atEnd = true;
		return true;
	}

	switch ( listDataType )
	{
	case RSSL_DT_INT :
		return getNextData( intList );
	case RSSL_DT_ASCII_STRING :
		return getNextData( stringList );
	default :
		{
			EmaString temp( "Unhandled search list data type." );

			// todo ... logger output to be provided
			_atEnd = true;
			return true;
		}
	}
}

bool FieldListDecoder::decodeViewList( RsslBuffer* rsslBuffer, RsslDataType& rsslDataType,
									  EmaVector< Int16 >& intList, EmaVector< EmaString >& stringList )
{
	RsslDecodeIterator dIter;
	rsslClearDecodeIterator( &dIter );
	rsslSetDecodeIteratorRWFVersion( &dIter, RSSL_RWF_MAJOR_VERSION, RSSL_RWF_MINOR_VERSION );
	rsslSetDecodeIteratorBuffer( &dIter, rsslBuffer );

	RsslElementList rsslElementList;
	rsslClearElementList( &rsslElementList );

	rsslDecodeElementList( &dIter, &rsslElementList, 0 );
	
	RsslElementEntry rsslElementEntry;
	rsslClearElementEntry( &rsslElementEntry );

	RsslRet retCode = rsslDecodeElementEntry( &dIter, &rsslElementEntry );
	bool searchListCompiled = false;

	while ( retCode == RSSL_RET_SUCCESS )
	{
		switch ( rsslElementEntry.dataType )
		{
		case RSSL_DT_ARRAY :
			{
				RsslArray rsslArray;
				rsslClearArray( &rsslArray );

				RsslDecodeIterator dIter;
				rsslClearDecodeIterator( &dIter );
				rsslSetDecodeIteratorRWFVersion( &dIter, RSSL_RWF_MAJOR_VERSION, RSSL_RWF_MINOR_VERSION );
				rsslSetDecodeIteratorBuffer( &dIter, &rsslElementEntry.encData );

				rsslDecodeArray( &dIter, &rsslArray );
				if ( rsslArray.primitiveType == RSSL_DT_INT )
				{
					RsslBuffer rsslBuffer;
					rsslClearBuffer( & rsslBuffer );
					RsslRet retCode = rsslDecodeArrayEntry( &dIter, &rsslBuffer );
					
					while ( retCode == RSSL_RET_SUCCESS )
					{
						RsslInt intValue;
						rsslDecodeInt( &dIter, &intValue );

						if ( intValue > 0xFFFF || intValue < -0xFFFF )
						{
							EmaString temp( "Out of range value specified in the search list. This value is dropped." );

							// todo ... logger output to be provided
						}
						else
						{
							if ( 0 > intList.getPositionOf( (Int16)intValue ) )
								intList.push_back( (Int16)intValue );
							retCode = rsslDecodeArrayEntry( &dIter, &rsslBuffer );
						}
					}

					if ( retCode != RSSL_RET_END_OF_CONTAINER )
					{
						EmaString temp( "Error decoding OmmArray with int while compiling search list." );

						// todo ...   logger output
					}
					else
					{
						searchListCompiled = true;
						rsslDataType = RSSL_DT_INT;
					}
				}
				else if ( rsslArray.primitiveType == RSSL_DT_ASCII_STRING )
				{
					RsslBuffer rsslBuffer;
					rsslClearBuffer( & rsslBuffer );
					RsslRet retCode = rsslDecodeArrayEntry( &dIter, &rsslBuffer );
					
					while ( retCode == RSSL_RET_SUCCESS )
					{
						RsslBuffer stringValue;
						rsslDecodeBuffer( &dIter, &stringValue );
						EmaString temp( stringValue.data, stringValue.length );
						if ( 0 > stringList.getPositionOf( temp ) )
							stringList.push_back( temp );
						retCode = rsslDecodeArrayEntry( &dIter, &rsslBuffer );
					}

					if ( retCode != RSSL_RET_END_OF_CONTAINER )
					{
						EmaString temp( "Error decoding OmmArray with strings while compiling search list." );

						// todo ...   logger output
					}
					else
					{
						searchListCompiled = true;
						rsslDataType = RSSL_DT_ASCII_STRING;
					}
				}
			}
			break;
		default :
			break;
		}

		retCode = rsslDecodeElementEntry( &dIter, &rsslElementEntry );
	}

	if ( retCode != RSSL_RET_END_OF_CONTAINER )
	{
		EmaString temp( "Error decoding ElementList while compiling a search list." );

		// todo logger output to be provided
		
		return false;
	}

	if ( !searchListCompiled )
	{
		EmaString temp( "Search list was not compiled." );

		// todo ... logger output to be provided
		return false;
	}

	return true;
}

const Data& FieldListDecoder::getLoad() const
{
	return _load;
}

void FieldListDecoder::setAtExit()
{
}

Int16 FieldListDecoder::getFieldId() const
{
	return _rsslFieldEntry.fieldId;
}

const EmaString& FieldListDecoder::getName()
{
	if ( !_rsslDictionaryEntry )
		_name.clear();
	else
		_name.setInt( _rsslDictionaryEntry->acronym.data, _rsslDictionaryEntry->acronym.length, true );

	return _name.toString();
}

Int16 FieldListDecoder::getRippleTo( Int16 fieldId ) const
{
	if ( !fieldId )
		fieldId = _rsslFieldEntry.fieldId;

	if ( !_pRsslDictionary->entriesArray[ fieldId ] )
		return 0;
	else
		return _pRsslDictionary->entriesArray[fieldId]->rippleToField;
}

bool FieldListDecoder::decodingStarted() const
{
	return _decodingStarted;
}

const EmaBuffer& FieldListDecoder::getHexBuffer()
{
	_hexBuffer.setFromInt( _rsslFieldListBuffer.data, _rsslFieldListBuffer.length );

	return _hexBuffer.toBuffer();
}