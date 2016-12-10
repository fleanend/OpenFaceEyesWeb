#include "stdafx.h"
#include "resource.h"
#include "Signature.h"

using namespace Eyw;

static catalog_class_registrant gCatalog( 
	catalog_class_registrant::catalog_id( EYW_OPENFACE_CATALOG_ID )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "OpenFace" )
			.description( "Implementation in Blocks of OpenFace's Framework" )
			.bitmap( IDB_OPENFACE_CATALOG )
		.end_language()	
	);

static company_registrant gCompany( 
	company_registrant::company_id( EYW_OPENFACE_COMPANY_ID )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "Dibris Unige" )
			.description( "University of Genoa" )
		.end_language()	
	);

static licence_registrant gLicense( 
	licence_registrant::licence_id( EYW_OPENFACE_LICENSE_ID )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "OpenFace Licence" )
			.description( "This catalog has the same license of EyesWeb XMI" )
		.end_language()	
	);

static author_registrant gAuthor( 
	author_registrant::author_id( EYW_OPENFACE_CATALOG_AUTHOR_ID )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "GrebeTeam" )
			.description( "Cool" )
		.end_language()	
	);


