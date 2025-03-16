class UBrowserEditFavoriteBox extends UWindowEditBox;

var UBrowserEditFavoriteCW CW;

function EditPaste()
{
    local bool bNotifyCW;
	
	if(bCanEdit)
	{
		if(bAllSelected || Len(Value) == 0)
		{
			Clear();
			bNotifyCW = true;
		}
		
		InsertText(GetPlayerOwner().PasteFromClipboard());

        if(bNotifyCW)
		    CW.NotifyEditPaste();
	}
}