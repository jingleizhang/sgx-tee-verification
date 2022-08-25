#include "sendgridmsg.h"

namespace SendGrid
{
    SendGridMessage::SendGridMessage()
    {
    }

    void SendGridMessage::setFrom(EmailAddress *email)
    {
        if (from)
        {
            delete from;
        }
        this->from = email;
    }

    void SendGridMessage::setReplyTo(EmailAddress *email)
    {
        if (replyTo)
        {
            delete replyTo;
        }
        this->replyTo = email;
    }
}