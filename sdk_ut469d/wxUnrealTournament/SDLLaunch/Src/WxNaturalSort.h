#include <wx/wx.h>
#include <wx/arrstr.h>

#if !(wxCHECK_VERSION(3, 1, 4))

// Used for comparison of string parts
struct wxStringFragment
{
    // Fragment types are generally sorted like this:
    // Empty < SpaceOrPunct < Digit < LetterOrSymbol
    // Fragments of the same type are compared as follows:
    // SpaceOrPunct - collated, Digit - as numbers using value
    // LetterOrSymbol - lower-cased and then collated
    enum Type
    {
        Empty,
        SpaceOrPunct,  // whitespace or punctuation
        Digit,         // a sequence of decimal digits
        LetterOrSymbol // letters and symbols, i.e., anything not covered by the above types
    };

    wxStringFragment() : type(Empty), value(0) {}

    Type     type;
    wxString text;
    wxUint64 value; // used only for Digit type
};


wxStringFragment GetFragment(wxString& text)
{
    if ( text.empty() )
        return wxStringFragment();

    // the maximum length of a sequence of digits that
    // can fit into wxUint64 when converted to a number
    static const ptrdiff_t maxDigitSequenceLength = 19;

    wxStringFragment         fragment;
    wxString::const_iterator it;

    for ( it = text.begin(); it != text.end(); ++it )
    {
        const wxUniChar&       ch = *it;
        wxStringFragment::Type chType = wxStringFragment::Empty;

        if ( wxIsspace(ch) || wxIspunct(ch) )
            chType = wxStringFragment::SpaceOrPunct;
        else if ( wxIsdigit(ch) )
            chType = wxStringFragment::Digit;
        else
            chType = wxStringFragment::LetterOrSymbol;

        // check if evaluating the first character
        if ( fragment.type == wxStringFragment::Empty )
        {
            fragment.type = chType;
            continue;
        }

        // stop processing when the current character has a different
        // string fragment type than the previously processed characters had
        // or a sequence of digits is too long
        if ( fragment.type != chType
             || (fragment.type == wxStringFragment::Digit
                 && it - text.begin() > maxDigitSequenceLength) )
        {
            break;
        }
    }

    fragment.text.assign(text.begin(), it);
    if ( fragment.type == wxStringFragment::Digit )
        fragment.text.ToULongLong(&fragment.value);

    text.erase(0, it - text.begin());

    return fragment;
}

int CompareFragmentNatural(const wxStringFragment& lhs, const wxStringFragment& rhs)
{
    switch ( lhs.type )
    {
        case wxStringFragment::Empty:
            switch ( rhs.type )
            {
                case wxStringFragment::Empty:
                    return 0;
                case wxStringFragment::SpaceOrPunct:
                case wxStringFragment::Digit:
                case wxStringFragment::LetterOrSymbol:
                    return -1;
            }
            break;

        case wxStringFragment::SpaceOrPunct:
            switch ( rhs.type )
            {
                case wxStringFragment::Empty:
                    return 1;
                case wxStringFragment::SpaceOrPunct:
                    return wxStrcoll_String(lhs.text, rhs.text);
                case wxStringFragment::Digit:
                case wxStringFragment::LetterOrSymbol:
                    return -1;
            }
            break;

        case wxStringFragment::Digit:
            switch ( rhs.type )
            {
                case wxStringFragment::Empty:
                case wxStringFragment::SpaceOrPunct:
                    return 1;
                case wxStringFragment::Digit:
                    if ( lhs.value >  rhs.value )
                        return 1;
                    else if ( lhs.value <  rhs.value )
                        return -1;
                    else
                        return 0;
                case wxStringFragment::LetterOrSymbol:
                    return -1;
            }
            break;

        case wxStringFragment::LetterOrSymbol:
            switch ( rhs.type )
            {
                case wxStringFragment::Empty:
                case wxStringFragment::SpaceOrPunct:
                case wxStringFragment::Digit:
                    return 1;
                case wxStringFragment::LetterOrSymbol:
                    return wxStrcoll_String(lhs.text.Lower(), rhs.text.Lower());
            }
            break;
    }

    // all possible cases should be covered by the switch above
    // but return also from here to prevent the compiler warning
    return 1;
}

int wxCMPFUNC_CONV wxCmpNaturalGeneric(const wxString& s1, const wxString& s2)
{
    wxString lhs(s1);
    wxString rhs(s2);

    int comparison = 0;

    while ( (comparison == 0) && (!lhs.empty() || !rhs.empty()) )
    {
        const wxStringFragment fragmentLHS = GetFragment(lhs);
        const wxStringFragment fragmentRHS = GetFragment(rhs);

        comparison = CompareFragmentNatural(fragmentLHS, fragmentRHS);
    }

    return comparison;
}

#endif
